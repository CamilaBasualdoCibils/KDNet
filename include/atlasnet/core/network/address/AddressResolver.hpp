#pragma once

#include "atlasnet/core/CoreDefs.hpp"
#include "atlasnet/core/network/address/SocketAddress.hpp"
#include "atlasnet/core/cache/Cache.hpp"
#include "atlasnet/core/client/ClientRegistry.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/EntityRegistry.hpp"
#include "atlasnet/core/node/NodeRegistry.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
#include "spdlog/logger.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"
#include <cassert>
#include <optional>
namespace AtlasNet
{

class AddressResolver
{

public:
  struct Config
  {
    NodeRegistry* nodeRegistry;
    ClientRegistry* clientRegistry;
    EntityRegistry* entityRegistry;
  };
  AddressResolver(const Config& config)
      : config_(config),
        clientAddressCache_([this](const AtlasNetClientID& id)
                            { return ClientAddressProvider(id); }),
        shardCache_([this](const AtlasNetShardID& id)
                    { return ShardAddressProvider(id); }),
        nodeCache_([this](const AtlasNetNodeID& id)
                   { return NodeAddressProvider(id); }),
        gatewayCache_([this](const AtlasNetGatewayID& id)
                      { return GatewayAddressProvider(id); }),
        clientGatewayCache_([this](const AtlasNetClientID& id)
                            { return ClientGatewayProvider(id); }),
        clientEntityCache_([this](const AtlasNetClientID& id)
                           { return ClientEntityIDProvider(id); },
                           [this](const AtlasNetEntityID& id)
                           { return EntityClientIDProvider(id); }),
        entityShardCache_([this](const AtlasNetEntityID& id)
                          { return EntityShardProvider(id); })
  {
    assert(config_.nodeRegistry != nullptr &&
           "NodeRegistry must not be nullptr");
    assert(config_.clientRegistry != nullptr &&
           "ClientRegistry must not be nullptr");
  }
  [[nodiscard]] std::optional<Network::SocketAddress>
  ResolveClientAddress(const AtlasNetClientID& clientID)
  {
    logger->info("Resolving client address for ID: {}", clientID.to_string());
    return clientAddressCache_.Get(clientID);
  }
  [[nodiscard]] std::optional<AtlasNetGatewayID>
  ResolveClientGateway(const AtlasNetClientID& clientID)
  {
    logger->info("Resolving client gateway for ID: {}", clientID.to_string());
    return clientGatewayCache_.Get(clientID);
  }
  [[nodiscard]] std::optional<Network::SocketAddress>
  ResolveShard(const AtlasNetShardID& shardID)
  {
    logger->info("Resolving shard address for ID: {}", shardID.to_string());
    return shardCache_.Get(shardID);
  }
  [[nodiscard]] std::optional<Network::SocketAddress>
  ResolveNode(const AtlasNetNodeID& nodeID)
  {
    logger->info("Resolving node address for ID: {}", nodeID.to_string());
    return nodeCache_.Get(nodeID);
  }
  [[nodiscard]] std::optional<Network::SocketAddress>
  ResolveGateway(const AtlasNetGatewayID& gatewayID)
  {
    logger->info("Resolving gateway address for ID: {}", gatewayID.to_string());
    return gatewayCache_.Get(gatewayID);
  }
  [[nodiscard]] std::optional<AtlasNetEntityID>
  ResolveClientEntityID(const AtlasNetClientID& clientID)
  {
    logger->info("Resolving entity ID for client ID: {}", clientID.to_string());
    return clientEntityCache_.GetByFirst(clientID);
  }
  [[nodiscard]] std::optional<AtlasNetClientID>
  ResolveEntityClientID(const AtlasNetEntityID& entityID)
  {
    logger->info("Resolving client ID for entity ID: {}", entityID.to_string());
    return clientEntityCache_.GetBySecond(entityID);
  }

  [[nodiscard]] std::optional<AtlasNetShardID>
  ResolveEntityShard(const AtlasNetEntityID& entityID)
  {
    logger->info("Resolving shard ID for entity ID: {}", entityID.to_string());
    return entityShardCache_.Get(entityID);
  }
  /*
   template <typename IDType>
   std::optional<SocketAddress> Resolve(const IDType& id)
   {
     if constexpr (std::is_same_v<IDType, ClientID>)
     {
       return ResolveClient(id);
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
   } */

private:
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("AddressResolver");
  Config config_;
  CacheMap<AtlasNetClientID, Network::SocketAddress> clientAddressCache_;
  CacheMap<AtlasNetClientID, AtlasNetGatewayID> clientGatewayCache_;
  CacheMap<AtlasNetShardID, Network::SocketAddress> shardCache_;
  CacheMap<AtlasNetNodeID, Network::SocketAddress> nodeCache_;
  CacheMap<AtlasNetGatewayID, Network::SocketAddress> gatewayCache_;
  CacheBiMap<AtlasNetClientID, AtlasNetEntityID> clientEntityCache_;
  CacheMap<AtlasNetEntityID, AtlasNetShardID> entityShardCache_;

  std::optional<Network::SocketAddress>
  GatewayAddressProvider(const AtlasNetGatewayID& gatewayID)
  {
    logger->warn("Retrieving non-cached gateway address for ID: {}",
                 gatewayID.to_string());
    return config_.nodeRegistry->ResolveAddress(gatewayID);
  }

  std::optional<Network::SocketAddress>
  ClientAddressProvider(const AtlasNetClientID& clientID)
  {
    logger->warn("Retrieving non-cached client address for ID: {}",
                 clientID.to_string());
    return std::nullopt;
  }
  std::optional<AtlasNetGatewayID>
  ClientGatewayProvider(const AtlasNetClientID& clientID)
  {
    logger->warn("Retrieving non-cached client gateway for ID: {}",
                 clientID.to_string());
    return std::nullopt;
  }
  std::optional<Network::SocketAddress>
  ShardAddressProvider(const AtlasNetShardID& shardID)
  {
    logger->warn("Retrieving non-cached shard address for ID: {}",
                 shardID.to_string());
    return config_.nodeRegistry->ResolveAddress(shardID);
  }
  std::optional<Network::SocketAddress> NodeAddressProvider(const AtlasNetNodeID& nodeID)
  {
    logger->warn("Retrieving non-cached node address for ID: {}",
                 nodeID.to_string());
    return config_.nodeRegistry->ResolveAddress(nodeID);
  }
  std::optional<AtlasNetClientID>
  EntityClientIDProvider(const AtlasNetEntityID& entityID)
  {
    logger->warn("Retrieving non-cached client ID for entity ID: {}",
                 entityID.to_string());
    return config_.clientRegistry->GetClientIDFromEntityID(entityID);
  }
  std::optional<AtlasNetEntityID>
  ClientEntityIDProvider(const AtlasNetClientID& clientID)
  {
    logger->warn("Retrieving non-cached entity ID for client ID: {}",
                 clientID.to_string());
    return config_.clientRegistry->GetClientEntityID(clientID);
  }
  std::optional<AtlasNetShardID>
  EntityShardProvider(const AtlasNetEntityID& entityID)
  {
    logger->warn("Retrieving non-cached shard ID for entity ID: {}",
                 entityID.to_string());
    return config_.entityRegistry->GetShardForEntity(entityID);
  }
};
} // namespace AtlasNet