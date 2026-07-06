#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/events/LocalEventSystem.hpp"
#include "atlasnet/core/messages/HandshakePacket.hpp"
#include "atlasnet/core/tasks/TaskSystem.hpp"
#include "atlasnet/core/utils/NetUtils.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "boost/stacktrace/stacktrace.hpp"
#include "enviroment/Enviroment.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/stacktrace.hpp>
#include <cassert>
#include <format>
#include <optional>
#include <sstream>
#include <string>
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

std::string get_hostname()
{
  char hostname[256] = {};
  if (::gethostname(hostname, sizeof(hostname)) != 0)
  {
    throw std::runtime_error("gethostname() failed");
  }
  hostname[sizeof(hostname) - 1] = '\0';
  return std::string(hostname);
}

bool AtlasNet::IService::ShutdownRequested() const
{
  return shutdown_requested.load(std::memory_order_acquire);
}

AtlasNet::IService::IService(ServiceType type)
    : type(type),
      logger(spdlog::stdout_color_mt(
          boost::describe::enum_to_string(type, "<INVALID_SERVICE_TYPE>")))

{
  auto handleShutdown = [](int)
  {
    std::shared_ptr<spdlog::logger> logger =
        spdlog::stdout_color_mt("ShutdownHandler");
    logger->info("SIGINT received, shutting down container...");
    IService::Get().shutdown_requested.store(true, std::memory_order_release);
    IService::Get().cv.notify_all();
  };

  auto HandleUnexpectedShutdown = [](int signal)
  {
    std::shared_ptr<spdlog::logger> logger =
        spdlog::stdout_color_mt("UnexpectedShutdownHandler");
    logger->error("Unexpected signal {} received, generating stack trace...",
                  signal);
    auto backtrace = boost::stacktrace::stacktrace();
    std::stringstream ss;
    ss << backtrace;
    logger->error("======== STACK TRACE ========\n{}\n====================",
                  ss.str());

    std::_Exit(1);
  };
  std::signal(SIGINT, handleShutdown);
  std::signal(SIGTERM, handleShutdown);

  std::signal(SIGSEGV, HandleUnexpectedShutdown);
  std::signal(SIGABRT, HandleUnexpectedShutdown);
}
void AtlasNet::IService::Init()
{
  logger->info("Container {} with ID {} starting up...",
               boost::describe::enum_to_string(type, "UNKNOWN"),
               GetContainerID().to_string());

  logger->info("Connecting to Redis database at {}:{}...",
               Env::DatabaseHostName, Env::DatabasePort);
  _redisDatabase = Database::RedisConn::Connect(Database::RedisConn::Settings{
      .host = HostAddress(Env::DatabaseHostName),
      .port = Env::DatabasePort,
      .Mode = Database::RedisConn::RedisMode::eStandalone,
      .ExceptionOnFailure = true,
      .MaxConnectRetries = 5,
      .ConnectRetryDelay = std::chrono::milliseconds(2000)});

  assert(_redisDatabase &&
         "Failed to connect to Redis database. Container cannot start.");
  _redisDatabase->KeyVal().GetSet().Set("container_id", get_hostname());
  _taskSystem.emplace(TaskSystem::Config{});
  _eventSystem.emplace(
      LocalEventSystem::Config{.taskSystem = &_taskSystem.value()});
  _globalEventSystem.emplace(GlobalEventSystem::Config{
      ._redisConn = _redisDatabase.get(), ._taskSystem = &_taskSystem.value()});
  _messageSystem.emplace(MessageSystem::Config{
      .taskSystem = &_taskSystem.value(),
      .localEventSystem = &_eventSystem.value(),
      .handshakeHandler = [this](const HandshakeIdentity& handshakeIdentity,
                                 const SocketAddress& remoteAddr)
      { return HandleHandshake(handshakeIdentity, remoteAddr); },
      .handshakeIdentity =
          HandshakeIdentity{
              .role = HandshakeRole::eServer,
              .data =
                  HandshakeServerRequestData{.serviceID = GetID(),
                                             .serviceType = GetServiceType()},
          },
  });
  _rpcSystem.emplace(
      RPCSystem::Config{.messageSystem = &_messageSystem.value()});
  _serviceRegistry.emplace(
      PresenceService::Config{.redisConn = _redisDatabase.get()});
  _universe.emplace(
      Universe::Config{._globalEventSystem = &_globalEventSystem.value(),
                       .__redisConfig = _redisDatabase.get()});
  _internalMessageSocket.emplace(
      &GetMessageSystem().OpenListenSocket(Env::InternalMessagePort));

  if (type != ServiceType::Controller)
  {

    FetchControllerInfo();
  }
  GetServiceRegistry().RegisterService(PresenceService::ServiceInfo{
      .id = GetContainerID(),
      .address = GetHostName(),
      .containerType = GetServiceType(),
  });

  OnInit();

  // conditional variable to wait until shutdown is requested, allowing for
  // clean shutdown when SIGINT is received

  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [this]
          { return shutdown_requested.load(std::memory_order_acquire); });
}
void AtlasNet::IService::FetchControllerInfo()
{
  while (true)
  {
    logger->info("Fetching Controller info from ServiceRegistry...");

    std::vector<PresenceService::ServiceInfo> outServices;
    GetServiceRegistry().GetServicesOfType(ServiceType::Controller,
                                           outServices);

    if (outServices.empty())
    {
      std::chrono::milliseconds retryDelay(500);
      logger->warn("No Controller service found. Agent initialization "
                   "failed. trying again in {}ms",
                   retryDelay.count());
      std::this_thread::sleep_for(retryDelay);
      continue;
    }
    const auto& controllerInfo = outServices[0];
    logger->info("Controller at {} with ID {}",
                 controllerInfo.address.to_string(),
                 controllerInfo.id.to_string());
    controllerOverlayAddress =
        SocketAddress(controllerInfo.address, Env::InternalMessagePort);
    controllerContainerID = controllerInfo.id;
  }
}
AtlasNet::HostAddress AtlasNet::IService::GetHostName() const
{
  if (const char* envHost = std::getenv("NODE_IP"))
  {
    logger->info("Using NODE_IP environment variable for hostname: {}",
                 envHost);
    return HostAddress(envHost);
  }
  else
  {
    logger->warn("NODE_IP environment variable not set. Unable to determine "
                 "Node Address. Attempting to use hostname.");
    char actualHost[256];
    if (gethostname(actualHost, sizeof(actualHost)) == 0)
    {
      return HostAddress(std::string(actualHost));
    }
    else
    {
      throw std::runtime_error(
          "Failed to retrieve hostname using gethostname().");
    }
  }
  return HostAddress();
}
