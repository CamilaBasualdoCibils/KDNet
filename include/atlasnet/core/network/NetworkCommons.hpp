#pragma once
#include "atlasnet/core/CoreDefs.hpp"
#include "boost/graph/properties.hpp"
#include <boost/graph/adjacency_list.hpp>
namespace AtlasNet
{
namespace Network
{

using NetworkNodeID = AtlasNetNodeID;
struct NetworkNodeInfo
{
  AtlasNetNodeID nodeID;
  std::string serverID;
  std::string rack;
  std::string region;
};
struct NetworkEdge
{
};
enum class SocketConnectionState : uint8_t
{
  eNone = 0,
  eConnecting = 1,
  eConnected = 2,
  eClosedByPeer = 3,
  eProblemDetectedLocally = 4,
  eINVALID = 5
};
enum class SocketSendMode : uint8_t
{
  eNoDelay = 0,
  eUnreliable = 1,
  eUnreliableBatched = 2,
  eReliable = 3,
  eReliableBatched = 4,
  eINVALID = 5
};

} // namespace Network
} // namespace AtlasNet