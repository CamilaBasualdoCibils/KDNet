#pragma once
#include "atlasnet/core/network/NetworkCommons.hpp"
#include "atlasnet/core/network/topology/TopologyCommons.hpp"
#include "atlasnet/core/network/NetworkCommons.hpp"
namespace AtlasNet::Network::Topology
{
class ITopologyExecutor
{
public:
  virtual ~ITopologyExecutor() = default;

  virtual bool SendConnectCommand(NetworkNodeID a, NetworkNodeID b) = 0;

  virtual bool SendDisconnectCommand(NetworkNodeID a, NetworkNodeID b) = 0;

  // Queries
  virtual AtlasNet::Network::SocketConnectionState GetConnectionState(NetworkNodeID a,
                                             NetworkNodeID b) const = 0;
};
} // namespace AtlasNet::Network::Topology