#include "AtlasNetNode.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "src/Node/ServiceDiscovery/RedisServiceDiscovery.hpp"
#include <cstdlib>
#include <unistd.h>

AtlasNet::AtlasNetNode::Options
AtlasNet::AtlasNetNode::ParseOptions(int argc, char** argv)
{
  Options options;
  po::options_description desc{"AtlasNetNode Options"};
  desc.add_options()("socket-type",
                     po::value<std::string>()->default_value("SteamNetSock"),
                     "Socket type (SteamNetSock, TCP, WebSocket)")(
      "ingress-port", po::value<uint16_t>()->default_value(7777),
      "Port for incoming connections")(
      "network-port", po::value<uint16_t>()->default_value(8888),
      "Port for network communications")
      // Redis
      ("redis-host",
       po::value<std::string>()->default_value(
           std::getenv("REDIS_HOST") ? std::getenv("REDIS_HOST") : "localhost"),
       "Redis host")(
          "redis-port",
          po::value<uint16_t>()->default_value(
              std::getenv("REDIS_PORT") ? std::atoi(std::getenv("REDIS_PORT"))
                                        : 6379),
          "Redis port")("redis-db", po::value<uint16_t>()->default_value(0),
                        "Redis database index")(
          "redis-user", po::value<std::string>()->default_value("default"),
          "Redis username")("redis-password",
                            po::value<std::string>()->default_value(""),
                            "Redis password");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    exit(0);
  }
  bool socketsuccess = boost::describe::enum_from_string(
      vm["socket-type"].as<std::string>(), options.socket_type);
  options.ingress_port = vm["ingress-port"].as<uint16_t>();
  options.network_port = vm["network-port"].as<uint16_t>();
  options.redis_address =
      Network::SocketAddress(vm["redis-host"].as<std::string>() + ":" +
                             std::to_string(vm["redis-port"].as<uint16_t>()));
  options.redis_db = vm["redis-db"].as<uint16_t>();
  options.redis_user = vm["redis-user"].as<std::string>();
  options.redis_password = vm["redis-password"].as<std::string>();
  return options;
}
void AtlasNet::AtlasNetNode::Initialize()
{
  logger->info("Initializing AtlasNet-Node[{}] - {}", nodeID.to_short_string(),
               nodeID.to_string());
  if (overrides.has_value())
  {
    if (overrides->transport_override)
    {
      transport = overrides->transport_override;
    }
    if (overrides->assigner_override)
    {
      topology_agent = std::make_unique<Network::Topology::TopologyAgent>(
          transport, std::move(overrides->assigner_override));
    }
  }

  if (!transport)
  {
    switch (options.socket_type)
    {
    case Network::SocketType::SteamNetSock:
      transport = std::make_unique<Network::SteamNetSockTransport>();
      break;
    case Network::SocketType::TCP:
      throw std::runtime_error("TCP transport not implemented yet");
      // transport = std::make_unique<Network::TCPTransport>();
      break;
    case Network::SocketType::WebSocket:
      throw std::runtime_error("WebSocket transport not implemented yet");
      // transport = std::make_unique<Network::WebSocketTransport>();
      break;
    default:
      throw std::runtime_error("Invalid socket type");
    }
  }
  if (!topology_agent)
  {
    topology_agent = std::make_unique<Network::Topology::TopologyAgent>(
        transport, std::make_unique<Network::Topology::RPCAssigner>());
  }
  {
    sw::redis::ConnectionOptions redis_options;
    redis_options.host = options.redis_address.to_host_address().to_string();
    redis_options.port = options.redis_address.get_port();
    redis_options.db = options.redis_db;
    redis_options.user = options.redis_user;
    redis_options.password = options.redis_password;
    logger->info("Connecting to Redis at {}:{}", redis_options.host,
                 redis_options.port);
    redis_client = std::make_shared<sw::redis::Redis>(redis_options);
    redis_async_client = std::make_shared<sw::redis::AsyncRedis>(redis_options);
    logger->info("Testing Redis connection... Ping...");
    std::string ping_response = redis_client->ping();
    logger->info("Redis ping response: {}", ping_response);
    if (ping_response.empty())
    {
      logger->error("Failed to connect to Redis at {}:{}", redis_options.host,
                    redis_options.port);
      throw std::runtime_error("Failed to connect to Redis");
    }
    logger->info("Connected to Redis at {}:{}", redis_options.host,
                 redis_options.port);

    service_discovery = std::make_unique<Service::RedisServiceDiscovery>(redis_client, redis_async_client);
    Service::ServiceData serviceData;
    serviceData.hostID = GetHostID();
    serviceData.internalAddress = Network::SocketAddress(GetNodeAddress(), options.network_port);
    
  }
}

std::string AtlasNet::AtlasNetNode::GetHostID() {
  return "INVALID";
}
AtlasNet::Network::HostAddress AtlasNet::AtlasNetNode::GetNodeAddress() {
    char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) != 0)
  {
    throw std::runtime_error("Failed to get hostname");
  }
  return Network::HostAddress(hostname);
}

int AtlasNet::AtlasNetNode::SetupSignals()
{
  sigset_t mask;
  sigemptyset(&mask);

  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGQUIT);
  if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0)
    throw std::runtime_error("pthread_sigmask failed");

  int signalFd = signalfd(-1, &mask, SFD_CLOEXEC);

  if (signalFd == -1)
    throw std::runtime_error("signalfd failed");
  return signalFd;
}
std::optional<int> AtlasNet::AtlasNetNode::CheckForSignal()
{
  signalfd_siginfo info;

  ssize_t n = read(signalFd, &info, sizeof(info));

  if (n != sizeof(info))
    return std::nullopt;
  return info.ssi_signo;
}
void AtlasNet::AtlasNetNode::MainLoop()
{
  logger->info("Starting main loop");
  while (!stop_requested.load())
  {
    std::optional<int> signal = CheckForSignal();
    if (signal.has_value())
    {
      switch (signal.value())
      {
      case SIGINT:
        logger->info("SIGINT: Ctrl+C");
        break;

      case SIGTERM:
        logger->info("SIGTERM");
        break;

      case SIGHUP:
        logger->info("SIGHUP: Reload config");
        break;

      case SIGQUIT:
        logger->info("SIGQUIT");
        break;
      }
      stop_requested.store(true);
      continue;
    }
    // Main loop logic here
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10ms = 100 Hz
  }
}

