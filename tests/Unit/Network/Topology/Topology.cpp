

#include "TopologyTestRunner.hpp"
#include "atlasnet/core/CoreDefs.hpp"
#include "atlasnet/core/network/NetworkCommons.hpp"
#include "atlasnet/core/network/topology/TopologyCommons.hpp"
#include "atlasnet/core/network/topology/cluster/ClusterNetworkTopology.hpp"
#include "atlasnet/core/network/topology/mesh/MeshNetworkTopology.hpp"
#include "atlasnet/core/network/topology/TopologyTransition.hpp"
#include "boost/graph/graphviz.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
using namespace AtlasNet::Network::Topology;
class MockTopologyExecutor : public ITopologyExecutor
{

public:
  MockTopologyExecutor() = default;
  MOCK_METHOD(bool, SendConnectCommand,
              (NetworkNodeID a, NetworkNodeID b),
              (override));

  MOCK_METHOD(bool, SendDisconnectCommand,
              (NetworkNodeID a, NetworkNodeID b),
              (override));
  MOCK_METHOD(AtlasNet::Network::SocketConnectionState, GetConnectionState,
              (NetworkNodeID a, NetworkNodeID b), (const, override));
};
TEST(Topology, TopologyTransition)
{
  using ID = AtlasNet::Network::NetworkNodeID;
  MockTopologyExecutor executor;
  TopologyTransition transition(executor);
  auto action0 = transition.AddConnectAction(ID(1), ID(2));
  auto action1 = transition.AddWaitConnectAction(ID(1), ID(2));
  transition.DependsOn(action0, action1);

  EXPECT_CALL(executor, SendConnectCommand(ID(1), ID(2))).Times(1);
  transition.Execute();
}
TEST(Topology, Mesh)
{
  AtlasNet::Network::Topology::MeshNetworkTopology topology;

  for (int i = 0; i < 5; ++i)
  {
    NetworkNodeInfo node{
        AtlasNet::AtlasNetNodeID(i), "server" + std::to_string(i),
        "rack" + std::to_string(i), "region" + std::to_string(i)};
    topology.AddNode(node);
  }
  topology.Parse();
  /*   topology.ComputeDiff(); */
}
struct VertexWriter
{
  const Topology::TopologyConnectionGraph& graph;

  VertexWriter(const Topology::TopologyConnectionGraph& g) : graph(g) {}

  template <typename Vertex>
  void operator()(std::ostream& out, const Vertex& v) const
  {
    const auto& node = graph[v];

    out << "[label=\"" << node.nodeID << "\\n" << node.serverID << "\"]";
  }
};
TEST(Topology, Cluster)
{

  AtlasNet::Network::Topology::ClusterNetworkTopology topology;
  const size_t numServers = 5;
  std::vector<std::string> servers;
  for (size_t i = 0; i < numServers; i++)
  {
    servers.push_back(std::format("Server {}", i));
  }

  const size_t numNodes = 50;
  for (size_t i = 0; i < numNodes; ++i)
  {
    NetworkNodeInfo node{AtlasNet::AtlasNetNodeID(i),
                         servers[i % servers.size()]

    };
    topology.AddNode(node);
  }

  topology.Parse();
  /*   Topology::TopologyDiff diff = topology.ComputeDiff();
    boost::write_graphviz(std::cout, topology.GetConnectionGraph(),
                          VertexWriter(topology.GetConnectionGraph())); */
}