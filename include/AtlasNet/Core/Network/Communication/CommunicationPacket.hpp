#pragma once

#include "AtlasNet/Core/Core.hpp"
namespace AtlasNet::Network
{
    enum class IntentMethod
{
  INVALID = 0,
  ShardOfEntity,
  ShardOfClient,
  IngressOfClient,
};
struct IntentHeader
{
  IntentMethod method;

  union
  {
    AtlasNetEntityID entityID;
    AtlasNetClientID clientID;
  };
  template <typename archive> void serialize(archive& ar)
  {
    ar(method);
    switch (method)
    {
    case IntentMethod::ShardOfEntity:
      ar(entityID);
      break;
    case IntentMethod::ShardOfClient:
      ar(clientID);
      break;
    default:
      break;
    }
  }
};
enum class NetworkProtocol : uint8_t
{
  INVALID = 0,
  Message = 1,
  RPC = 2,
  Telemetry = 3,
};

enum class ACKMode : uint8_t
{
  None = 0b00,    // fire and forget
  Request = 0b01, // please acknowledge receipt
  Response = 0b11 // this packet is an acknowledgement
};

struct CommunicationHeaderV1
{
  NetworkProtocol protocol;
  AtlasNetNodeID origin;
  uint64_t correlationID;
  ACKMode ackMode;
  IntentHeader intent;
  template <typename archive> void serialize(archive& ar)
  {
    ar(protocol, origin, correlationID, ackMode, intent);
  }
};
} // namespace AtlasNet::Network