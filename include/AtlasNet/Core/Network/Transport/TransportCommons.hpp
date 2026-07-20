#pragma once

#include "boost/describe/enum.hpp"
#include <cstdint>
#include <boost/describe.hpp>
namespace AtlasNet::Network
{
enum class SocketConnectionState : uint8_t
{
  None = 0,
  Connecting = 1,
  Connected = 2,
  ClosedByPeer = 3,
  ProblemDetectedLocally = 4,
  INVALID = 5
};
BOOST_DESCRIBE_ENUM(SocketConnectionState, None, Connecting, Connected, ClosedByPeer, ProblemDetectedLocally, INVALID)
enum class SocketSendMode : uint8_t
{
  NoDelay = 0,
  Unreliable = 1,
  UnreliableBatched = 2,
  Reliable = 3,
  ReliableBatched = 4,
  INVALID = 5
};
BOOST_DESCRIBE_ENUM(SocketSendMode, NoDelay, Unreliable, UnreliableBatched, Reliable, ReliableBatched, INVALID)
enum class SocketType : uint8_t
{
  TCP = 0,
  WebSocket = 1,
  SteamNetSock = 2,
  INVALID = 3
};
BOOST_DESCRIBE_ENUM(SocketType, TCP, WebSocket, SteamNetSock, INVALID)
} // namespace AtlasNet::Network