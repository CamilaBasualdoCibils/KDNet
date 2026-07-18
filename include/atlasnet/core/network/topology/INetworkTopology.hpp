#pragma once
#include "atlasnet/core/network/NetworkCommons.hpp"
#include "atlasnet/core/network/topology/TopologyCommons.hpp"
#include "atlasnet/core/network/topology/TopologyTransition.hpp"
namespace AtlasNet::Network::Topology
{
class INetworkTopology
{
public:
  virtual ~INetworkTopology() = default;

  virtual void AddNode(const NetworkNodeInfo&) = 0;

  virtual void RemoveNode(const NetworkNodeID&) = 0;

  virtual void Parse() = 0;

  virtual TopologyTransition
  CreateTransition(ITopologyExecutor& executor, const TopologyConnectionGraph&) = 0;
  /* virtual TopologyDiff ComputeDiff() = 0; */

  virtual const TopologyWeightedGraph& GetWeightedGraph() const = 0;
  virtual const TopologyConnectionGraph& GetConnectionGraph() const = 0;
};
} // namespace AtlasNet::Network::Topology