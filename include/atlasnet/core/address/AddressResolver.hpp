#pragma once

#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/cache/Cache.hpp"
#include "atlasnet/core/entity/Entity.hpp"
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
                            { return ClientGatewayProvider(id); })
  {
    assert(config_.nodeRegistry != nullptr &&
           "NodeRegistry must not be nullptr");
  }
  std::optional<SocketAddress>
  ResolveClientAddress(const AtlasNetClientID& clientID)
  {
    logger->info("Resolving client address for ID: {}", clientID.to_string());
    return clientAddressCache_.Get(clientID);
  }
  std::optional<AtlasNetGatewayID>
  ResolveClientGateway(const AtlasNetClientID& clientID)
  {
    logger->info("Resolving client gateway for ID: {}", clientID.to_string());
    return clientGatewayCache_.Get(clientID);
  }
  std::optional<SocketAddress> ResolveShard(const AtlasNetShardID& shardID)
  {
    logger->info("Resolving shard address for ID: {}", shardID.to_string());
    return shardCache_.Get(shardID);
  }
  std::optional<SocketAddress> ResolveNode(const AtlasNetNodeID& nodeID)
  {
    logger->info("Resolving node address for ID: {}", nodeID.to_string());
    return nodeCache_.Get(nodeID);
  }
  std::optional<SocketAddress>
  ResolveGateway(const AtlasNetGatewayID& gatewayID)
  {
    logger->info("Resolving gateway address for ID: {}", gatewayID.to_string());
    return gatewayCache_.Get(gatewayID);
  } /*
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
  CacheMap<AtlasNetClientID, SocketAddress> clientAddressCache_;
  CacheMap<AtlasNetClientID, AtlasNetGatewayID> clientGatewayCache_;
  CacheMap<AtlasNetShardID, SocketAddress> shardCache_;
  CacheMap<AtlasNetNodeID, SocketAddress> nodeCache_;
  CacheMap<AtlasNetGatewayID, SocketAddress> gatewayCache_;

  std::optional<SocketAddress>
  GatewayAddressProvider(const AtlasNetGatewayID& gatewayID)
  {
    logger->warn("Retrieving non-cached gateway address for ID: {}",
                 gatewayID.to_string());
    return config_.nodeRegistry->ResolveAddress(gatewayID);
  }

  std::optional<SocketAddress>
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
  std::optional<SocketAddress>
  ShardAddressProvider(const AtlasNetShardID& shardID)
  {
    logger->warn("Retrieving non-cached shard address for ID: {}",
                 shardID.to_string());
    return config_.nodeRegistry->ResolveAddress(shardID);
  }
  std::optional<SocketAddress> NodeAddressProvider(const AtlasNetNodeID& nodeID)
  {
    logger->warn("Retrieving non-cached node address for ID: {}",
                 nodeID.to_string());
    return config_.nodeRegistry->ResolveAddress(nodeID);
  }
};
} // namespace AtlasNet