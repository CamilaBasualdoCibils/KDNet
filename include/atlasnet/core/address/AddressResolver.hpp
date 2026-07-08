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
        nodeCache([this](const AtlasNetNodeID& id)
                  { return NodeAddressProvider(id); }),
        gatewayCache_([this](const AtlasNetGatewayID& id)
                      { return GatewayAddressProvider(id); })
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
  std::optional<SocketAddress> ResolveNode(const AtlasNetNodeID& nodeID)
  {
    return nodeCache.Get(nodeID);
  }
  std::optional<SocketAddress>
  ResolveGateway(const AtlasNetGatewayID& gatewayID)
  {
    return gatewayCache_.Get(gatewayID);
  }
  template <typename IDType>
  std::optional<SocketAddress> Resolve(const IDType& id)
  {
    if constexpr (std::is_same_v<IDType, ClientID>)
    {
      return ResolveClient(id);
    }
    else if constexpr (std::is_same_v<IDType, EntityID>)
    {
      return ResolveEntity(id);
    }
    else if constexpr (std::is_same_v<IDType, AtlasNetShardID>)
    {
      return ResolveShard(id);
    }
    else if constexpr (std::is_same_v<IDType, AtlasNetNodeID>)
    {
      return ResolveNode(id);
    }
    else if constexpr (std::is_same_v<IDType, AtlasNetGatewayID>)
    {
      return ResolveGateway(id);
    }
    else
    {
      static_assert(false, "Unsupported ID type");
    }
  }

private:
  Config config_;
  Cache<ClientID, SocketAddress> clientCache_;
  Cache<EntityID, SocketAddress> entityCache_;
  Cache<AtlasNetShardID, SocketAddress> shardCache_;
  Cache<AtlasNetNodeID, SocketAddress> nodeCache;
  Cache<AtlasNetGatewayID, SocketAddress> gatewayCache_;

  std::optional<SocketAddress>
  GatewayAddressProvider(const AtlasNetGatewayID& gatewayID)
  {
    return std::nullopt;
  }

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
  std::optional<SocketAddress> NodeAddressProvider(const AtlasNetNodeID& nodeID)
  {
    return std::nullopt;
  }
};
} // namespace AtlasNet