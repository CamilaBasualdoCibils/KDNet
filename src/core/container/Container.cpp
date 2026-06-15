#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/events/LocalEventSystem.hpp"
#include "atlasnet/core/messages/HandshakePacket.hpp"
#include "atlasnet/core/utils/NetUtils.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "boost/stacktrace/stacktrace.hpp"
#include "enviroment/Enviroment.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/stacktrace.hpp>
#include <cassert>
#include <format>
#include <optional>
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
  return shutdown.load(std::memory_order_acquire);
}

AtlasNet::IService::IService(ServiceType type) : type(type)
{
  auto handleShutdown = [](int)
  {
    std::cerr << "SIGINT received, shutting down container..." << std::endl;
    IService::Get().shutdown.store(true, std::memory_order_release);
    IService::Get().cv.notify_all();
  };

  auto HandleUnexpectedShutdown = [](int signal)
  {
    std::cerr << "Unexpected signal " << signal
              << " received, generating stack trace..." << std::endl;
    auto backtrace = boost::stacktrace::stacktrace();
    ;
    std::cerr << "======== STACK TRACE ========\n"
              << backtrace << "\n====================\n";
    std::_Exit(1);
  };
  std::signal(SIGINT, handleShutdown);
  std::signal(SIGTERM, handleShutdown);

  std::signal(SIGSEGV, HandleUnexpectedShutdown);
  std::signal(SIGABRT, HandleUnexpectedShutdown);
}
void AtlasNet::IService::Init()
{
  std::cerr << std::format("Container {} with ID {} starting up...",
                           boost::describe::enum_to_string(type, "UNKNOWN"),
                           GetContainerID().to_string())
            << std::endl;
  std::cerr << "Container hostname: " << GetHostName().to_string() << std::endl;
  if (const char* test_port = std::getenv("ATLASNET_DATABASE_PORT"); test_port)
  {
    std::cerr << "Database port from environment: " << test_port << std::endl;
  }
  std::cerr << "Connecting to Redis database at " << Env::DatabaseHostName
            << ":" << Env::DatabasePort << "..." << std::endl;
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
  _jobSystem.emplace(JobSystem::Config{});
  _eventSystem.emplace(
      LocalEventSystem::Config{.jobSystem = &_jobSystem.value()});
  _globalEventSystem.emplace(GlobalEventSystem::Config{
      ._redisConn = _redisDatabase.get(), ._jobSystem = &_jobSystem.value()});
  _messageSystem.emplace(MessageSystem::Config{
      .jobSystem = &_jobSystem.value(),
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
      ServiceRegistry::Config{.redisConn = _redisDatabase.get()});
  _universe.emplace(
      Universe::Config{._globalEventSystem = &_globalEventSystem.value(),
                       .__redisConfig = _redisDatabase.get()});
  _internalMessageSocket.emplace(
      &GetMessageSystem().OpenListenSocket(Env::InternalMessagePort));

  if (type != ServiceType::Controller)
  {

    FetchControllerInfo();
  }
  OnInit();
  while (!ShutdownRequested())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
  }
}
void AtlasNet::IService::FetchControllerInfo()
{
  JobHandle jobHandle = GetJobSystem().Submit(
      [this](JobContext& ctx)
      {
        std::cerr << "Fetching Controller info from ServiceRegistry..."
                  << std::endl;

        std::vector<ServiceRegistry::ServiceInfo> outServices;
        GetServiceRegistry().GetServicesOfType(ServiceType::Controller,
                                               outServices);

        if (outServices.empty())
        {
          std::chrono::milliseconds retryDelay(500);
          std::cerr << "No Controller service found. Agent initialization "
                       "failed. trying again in "
                    << retryDelay.count() << "ms" << std::endl;
          ctx.repeat_once(retryDelay);
          return;
        }
        const auto& controllerInfo = outServices[0];
        std::cerr << "Controller at " << controllerInfo.address.to_string()
                  << " with ID " << controllerInfo.id.to_string() << std::endl;
        controllerOverlayAddress =
            SocketAddress(controllerInfo.address, Env::InternalMessagePort);
        controllerContainerID = controllerInfo.id;
      });

  jobHandle.wait();
}
AtlasNet::HostAddress AtlasNet::IService::GetHostName() const
{
  if (const char* envHost = std::getenv("NODE_IP"))
  {
    std::cerr << "Using NODE_IP environment variable for hostname: " << envHost
              << std::endl;
    return HostAddress(envHost);
  }
  else
  {
    std::cerr << "NODE_IP environment variable not set. Unable to determine "
                 "Node Address. Attempting to use hostname."
              << std::endl;
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
