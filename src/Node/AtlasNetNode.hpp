#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Topology/Assigner/RPCAssigner.hpp"
#include "AtlasNet/Core/Network/Topology/TopologyAgent.hpp"
#include "AtlasNet/Core/Network/Transport/ITransport.hpp"
#include "AtlasNet/Core/Network/Transport/SteamNetSock/SteamNetSock.hpp"
#include "AtlasNet/Core/Network/Transport/TransportCommons.hpp"
#include "boost/describe/enum_from_string.hpp"
#include "src/Node/ServiceDiscovery/IServiceDiscovery.hpp"
#include "sw/redis++/connection.h"
#include "sw/redis++/redis.h"
#include "sw/redis++/redis_cluster.h"
#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>
#include <csignal>
#include <iostream>
#include <sw/redis++/async_redis.h>
#include <sys/signalfd.h>
namespace po = boost::program_options;
namespace AtlasNet
{
class AtlasNetNode
{
private:
  struct Options
  {
    Network::SocketType socket_type;

    uint16_t ingress_port;
    uint16_t network_port;
    Network::SocketAddress redis_address;
    uint16_t redis_db;
    std::string redis_user;
    std::string redis_password;
  } options;
  struct Overrides
  {
    std::shared_ptr<Network::ITransport> transport_override;
    std::unique_ptr<Network::Topology::ITopologyAssigner> assigner_override;
  };
  std::optional<Overrides> overrides;

  std::atomic_bool stop_requested{false};
  
  //Redis
  std::shared_ptr<sw::redis::Redis> redis_client;
  std::shared_ptr<sw::redis::AsyncRedis> redis_async_client;
  
  std::shared_ptr<Network::ITransport> transport;
  std::unique_ptr<Network::Topology::TopologyAgent> topology_agent;
  std::unique_ptr<Service::IServiceDiscovery> service_discovery;
  std::shared_ptr<spdlog::logger> logger;
  const int signalFd;
  const AtlasNetNodeID nodeID;

public:
  AtlasNetNode(int argc, char** argv)
      : options(ParseOptions(argc, argv)), signalFd(SetupSignals()),
        logger(spdlog::stdout_color_mt("AtlasNetNode")),
        nodeID(UUID::Generate())
  {
  }
  AtlasNetNode(Options options, Overrides overrides)
      : options(std::move(options)), signalFd(SetupSignals()),
        logger(spdlog::stdout_color_mt("AtlasNetNode")),
        overrides(std::move(overrides)), nodeID(UUID::Generate())
  {
  }
  void Run()
  {
    // Initialize the node
    Initialize();

    // Start the main loop
    MainLoop();
  }

private:
  static Options ParseOptions(int argc, char** argv);
  void Initialize();
  void MainLoop();
  static std::string GetHostID();
  static AtlasNet::Network::HostAddress GetNodeAddress();
  static int SetupSignals();
   std::optional<int> CheckForSignal();
};
} // namespace AtlasNet