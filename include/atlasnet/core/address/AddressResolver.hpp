#pragma once

#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/cache/Cache.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
#include <optional>
namespace AtlasNet
{

class AddressResolver
{

public:
  struct Config
  {
  };
  AddressResolver(const Config& config)
      : config_(config), clientCache_([this](const ClientID& id)
                                      { return ClientAddressProvider(id); }),
        entityCache_([this](const EntityID& id)
                     { return EntityAddressProvider(id); }),
        shardCache_([this](const AtlasNetShardID& id)
                    { return ShardAddressProvider(id); }),
        serviceCache_([this](const AtlasNetNodeID& id)
                      { return ServiceAddressProvider(id); })
  {
  }
  std::optional<SocketAddress> ResolveClient(const ClientID& clientID)
  {
    return clientCache_.Get(clientID);
  }
  std::optional<SocketAddress> ResolveEntity(const EntityID& entityID)
  {
    return entityCache_.Get(entityID);
  }
  std::optional<SocketAddress> ResolveShard(const AtlasNetShardID& shardID)
  {
    return shardCache_.Get(shardID);
  }
  std::optional<SocketAddress> ResolveService(const AtlasNetNodeID& serviceID)
  {
    return serviceCache_.Get(serviceID);
  }

private:
  Config config_;
  Cache<ClientID, SocketAddress> clientCache_;
  Cache<EntityID, SocketAddress> entityCache_;
    Cache<AtlasNetShardID, SocketAddress> shardCache_;
      Cache<AtlasNetNodeID, SocketAddress> serviceCache_;
      
  std::optional<SocketAddress> ClientAddressProvider(const ClientID& clientID)
  {
    return std::nullopt;
  }
  std::optional<SocketAddress> EntityAddressProvider(const EntityID& entityID)
  {
    return std::nullopt;
  }
  std::optional<SocketAddress>
  ShardAddressProvider(const AtlasNetShardID& shardID)
  {
    return std::nullopt;
  }
  std::optional<SocketAddress>
  ServiceAddressProvider(const AtlasNetNodeID& serviceID)
  {
    return std::nullopt;
  }

};
} // namespace AtlasNet