#include "AtlasNetNode.hpp"
#include "AtlasNet/Core/Lease/RedisLeaseProvider.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Node/NodeData.hpp"
#include "Core/Events/RedisGlobalEvents.hpp"
#include "Network/Interface/LinuxNetworkInterface.hpp"
#include "Node/ServiceDiscovery/RedisServiceDiscovery.hpp"
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
      ("db-host",
       po::value<std::string>()->default_value(
           std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost"),
       "Database host")(
          "db-port",
          po::value<uint16_t>()->default_value(
              std::getenv("DB_PORT") ? std::atoi(std::getenv("DB_PORT"))
                                     : 6379),
          "Database port");
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
  options.db_address =
      Network::SocketAddress(vm["db-host"].as<std::string>() + ":" +
                             std::to_string(vm["db-port"].as<uint16_t>()));
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

  if (!network_interface)
  {
    network_interface =
        std::make_shared<Network::LinuxNetworkInterface>("eno1");
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
  logger->info("Connecting to database at {}",
               options.db_address.to_string());
  DBConnection = transport->Connect(options.db_address);
  /*
  {
  
  
     global_events =
        std::make_shared<Events::RedisGlobalEvents>(redis_async_client);

    service_discovery =
        std::make_unique<Service::RedisServiceDiscovery>(redis_client); 
    NodeData serviceData;
    serviceData.hostID = GetHostID();
    serviceData.internalAddress =
        Network::SocketAddress(GetNodeAddress(), options.network_port);
    serviceData.macAddress = network_interface->GetMACAddress();
    serviceData.nodeID = nodeID;
    service_lease = service_discovery->RegisterService(serviceData);
    lease_provider = std::make_shared<RedisLeaseProvider>(nodeID, redis_client);
    {
      auto controller_lease_future =
          lease_provider->ClaimOrGetLeaseAsync(ControllerLeaseKey);
      auto controller_lease_result = controller_lease_future.get();
      if (controller_lease_result.has_value())
      {
        if (std::holds_alternative<std::unique_ptr<ILease>>(controller_lease_result.value()))
        {
          controller_lease = std::move(
              std::get<std::unique_ptr<ILease>>(controller_lease_result.value()));
          logger->info("Successfully claimed controller lease");
        }
        else if (std::holds_alternative<LeaseInfo>(controller_lease_result.value()))
        {
          LeaseInfo info =
              std::get<LeaseInfo>(controller_lease_result.value());
          logger->info(
              "Controller lease is already held by another node: {}",
              info.Owner.to_string());
        }
      }
    }
  }*/
}

std::string AtlasNet::AtlasNetNode::GetHostID()
{
  return "INVALID";
}
AtlasNet::Network::HostAddress AtlasNet::AtlasNetNode::GetNodeAddress()
{
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

  int signalFd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);

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
        default:
          logger->info("Received signal: {}. Ignoring.", signal.value());
          break;
      }
      stop_requested.store(true);
      continue;
    }
    // Main loop logic here
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 10ms = 100 Hz
  }
}
