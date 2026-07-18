#pragma once
#include "atlasnet/core/CoreDefs.hpp"
#include "atlasnet/core/network/NetworkCommons.hpp"
#include <boost/graph/adjacency_list.hpp>

namespace AtlasNet::Network::Topology
{
enum class TopologyConnectionState
{
  Connected,
  Disconnected,
  Connecting,
  Disconnecting
};
using TopologyWeightedGraph = boost::adjacency_list<
    boost::vecS,                                 // vertex storage
    boost::vecS,                                 // edge storage
    boost::undirectedS,                          // undirected graph
    NetworkNodeInfo,                             // vertex properties
    boost::property<boost::edge_weight_t, float> // edge properties
    >;
using TopologyWeightedVertex =
    boost::graph_traits<TopologyWeightedGraph>::vertex_descriptor;
using TopologyWeightedEdge =
    boost::graph_traits<TopologyWeightedGraph>::edge_descriptor;

using TopologyConnectionGraph =
    boost::adjacency_list<boost::vecS,        // vertex storage
                          boost::vecS,        // edge storage
                          boost::undirectedS, // undirected graph
                          NetworkNodeInfo,    // vertex properties
                          NetworkEdge         // edge properties
                          >;
using TopologyConnectionVertex =
    boost::graph_traits<TopologyConnectionGraph>::vertex_descriptor;
using TopologyConnectionEdge =
    boost::graph_traits<TopologyConnectionGraph>::edge_descriptor;
struct TopologyDiff
{
  std::vector<TopologyConnectionEdge> connect;
  std::vector<TopologyConnectionEdge> disconnect;
};
struct TopologyActionResult
{
  bool success;
  std::string message;
};
using TopologyActionCallback = std::function<void(TopologyActionResult)>;
} // namespace AtlasNet::Network::Topology
