#pragma once
#include "AtlasNet/Core/Core.hpp"
#include "boost/graph/properties.hpp"
#include <boost/graph/adjacency_list.hpp>
namespace AtlasNet
{
namespace Network
{


struct NetworkNodeInfo
{
  AtlasNetNodeID nodeID;
  float BaseWeight = 0.0f;
  std::string serverID;
  std::string rack;
  std::string region;
  uint32_t DesiredConnections;
};
struct NetworkEdge
{
};


} // namespace Network
} // namespace AtlasNet