#pragma once

#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/cache/Cache.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/shard/shard.hpp"
#include <optional>
namespace AtlasNet
{

class AddressResolver
{

public:
  struct Config
  {
  };
  AddressResolver(const Config& config) {}
  std::optional<SocketAddress> ResolveClient(const ClientID& clientID)
  {
    return clientCache_.FindEnsured(
        clientID, [this](const ClientID& clientID)
        { return ClientAddressProvider(clientID); });
  }
  std::optional<SocketAddress> ResolveEntity(const EntityID& entityID)
  {
    return entityCache_.FindEnsured(
        entityID, [this](const EntityID& entityID)
        { return EntityAddressProvider(entityID); });
  }
  std::optional<SocketAddress> ResolveShard(const ShardID& shardID)
  {
    return shardCache_.FindEnsured(shardID, [this](const ShardID& shardID)
                                   { return ShardAddressProvider(shardID); });
  }
  std::optional<SocketAddress> ResolveService(const ServiceID& serviceID)
  {
    return serviceCache_.FindEnsured(
        serviceID, [this](const ServiceID& serviceID)
        { return ServiceAddressProvider(serviceID); });
  }

private:
  Config config_;
  std::optional<SocketAddress> ClientAddressProvider(const ClientID& clientID)
  {
    return std::nullopt;
  }
  Cache<ClientID, SocketAddress> clientCache_;

  std::optional<SocketAddress> EntityAddressProvider(const EntityID& entityID)
  {
    return std::nullopt;
  }
  Cache<EntityID, SocketAddress> entityCache_;

  std::optional<SocketAddress> ShardAddressProvider(const ShardID& shardID)
  {
    return std::nullopt;
  }
  Cache<ShardID, SocketAddress> shardCache_;

  std::optional<SocketAddress>
  ServiceAddressProvider(const ServiceID& serviceID)
  {
    return std::nullopt;
  }
  Cache<ServiceID, SocketAddress> serviceCache_;
};
} // namespace AtlasNet