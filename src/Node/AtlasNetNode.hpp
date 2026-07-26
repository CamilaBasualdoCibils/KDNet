#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Events/IGlobalEvents.hpp"
#include "AtlasNet/Core/Lease/ILeaseProvider.hpp"
#include "AtlasNet/Core/Network/Address/MacAddress.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Interface/INetworkInterface.hpp"
#include "AtlasNet/Core/Network/Topology/Assigner/RPCAssigner.hpp"
#include "AtlasNet/Core/Network/Topology/TopologyAgent.hpp"
#include "AtlasNet/Core/Network/Transport/Connection/IConnection.hpp"
#include "AtlasNet/Core/Network/Transport/Connection/IConnectionListener.hpp"
#include "AtlasNet/Core/Network/Transport/Connection/IConnectionTransport.hpp"
#include "AtlasNet/Core/Network/Transport/Datagram/IDatagramTransport.hpp"
#include "AtlasNet/Core/Network/Transport/TransportCommons.hpp"
#include "AtlasNet/Core/Service/AtlasNetService.hpp"
#include "Node/ServiceDiscovery/IServiceDiscovery.hpp"
#include "boost/describe/enum_from_string.hpp"
#include "sw/redis++/connection.h"
#include "sw/redis++/redis.h"
#include "sw/redis++/redis_cluster.h"

#include <csignal>
#include <iostream>
#include <sw/redis++/async_redis.h>
#include <sys/signalfd.h>
#include <variant>

namespace AtlasNet
{
class AtlasNetNode : public AtlasNetService
{
  struct NodeConfig
  {
    Network::SocketAddress dbAddress;
  };
  NodeConfig nodeConfig;
private:
public:
  AtlasNetNode(int argc, char** argv)
      : AtlasNetService(AtlasNetServiceType::Node, argc, argv)
  {
  }

private:
  void ParseOptions(const boost::program_options::variables_map& vm) override;
  void AddOptions(boost::program_options::options_description& desc) override;
  void Initialize() override {}

  void Tick() override {}
};
} // namespace AtlasNet