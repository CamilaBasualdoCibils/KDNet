#pragma once

#include "AtlasNet/Core/Network/Topology/TopologyAgent.hpp"
#include "AtlasNet/Core/Network/Transport/IListener.hpp"
#include "AtlasNet/Core/Network/Transport/ITransport.hpp"
#include "AtlasNet/Core/Network/Transport/TransportCommons.hpp"
#include "Network/Interface/INetworkInterface.hpp"
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
namespace AtlasNet
{
class AtlasNetDB
{
struct Options
  {
    Network::SocketType socket_type = Network::SocketType::INVALID;
    uint16_t network_port;

  } options;
  std::shared_ptr<Network::INetworkInterface> network_interface;
  std::shared_ptr<Network::ITransport> transport;
  std::shared_ptr<Network::IListener> listener;
  //std::unique_ptr<Network::Topology::TopologyAgent> topology_agent;
  

  std::shared_ptr<spdlog::logger> logger;

public:
  AtlasNetDB() : logger(spdlog::stdout_color_mt("AtlasNetDB")) {}

  void Run()
  {
    // Initialize the node
    Initialize();

    // Start the main loop
    MainLoop();
  }

private:
  void Initialize();
  void MainLoop();
  void HandleConnectionRequest(Network::ConnectionRequest&,
                               Network::IListener&);
};
}; // namespace AtlasNet