#pragma once

#include "AtlasNet/Core/Core.hpp"
namespace AtlasNet::Network::Messages
{
class IRecepient
{
};
namespace Recepient
{
class NodeRecepient : public IRecepient
{
  AtlasNetNodeID nodeID;

public:
  NodeRecepient(const AtlasNetNodeID& nodeID) : nodeID(nodeID) {}
};
class ShardOfEntityRecepient : public IRecepient
{
  AtlasNetEntityID entityID;

public:
  ShardOfEntityRecepient(const AtlasNetEntityID& entityID) : entityID(entityID)
  {
  }
};
class ShardOfClientRecepient : public IRecepient
{
  AtlasNetClientID clientID;

public:
  ShardOfClientRecepient(const AtlasNetClientID& clientID) : clientID(clientID)
  {
  }
};
class DatabaseRecepient : public IRecepient
{
  // Database specific fields can be added here
public:
  DatabaseRecepient() {}
};
} // namespace Recepient

using VRecepient =
    std::variant<Recepient::NodeRecepient, Recepient::ShardOfEntityRecepient,
                 Recepient::ShardOfClientRecepient,
                 Recepient::DatabaseRecepient>;
} // namespace AtlasNet::Network::Messages