#pragma once

#include "atlasnet/core/CoreDefs.hpp"
#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/cache/Cache.hpp"
#include "atlasnet/core/client/ClientRegistry.hpp"

#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/CmdSig/command/Command.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"

#include "enviroment/Enviroment.hpp"
#include "spdlog/sinks/stdout_color_sinks-inl.h"
#include <cassert>
#include <shared_mutex>

namespace AtlasNet
{
class AtlasNetGateway;
class GatewayRelayService
{

public:
  struct Config
  {
    Database::RedisConn* redisConn;
    AtlasNetGateway* gateway;
    MessageSystem* messageSystem;
    ClientRegistry* clientRegistry;
    // Add any necessary configuration parameters here
  };
  GatewayRelayService(const Config& config)
      : config_(config),
        clientToAddressCache_([this](const SocketAddress& address)
                              { return __GetClientIDByAddress(address); },
                              [this](const AtlasNetClientID& id)
                              { return __GetClientAddressByID(id); }),
        clientToShardCache_([this](const AtlasNetClientID& id)
                            { return __GetShardByClientID(id); })
  {
    assert(config_.redisConn && "RedisConn pointer cannot be null");
    assert(config_.gateway && "Gateway pointer cannot be null");
    assert(config_.messageSystem && "MessageSystem pointer cannot be null");
    assert(config_.clientRegistry && "ClientRegistry pointer cannot be null");

    config_.messageSystem->On<ExternalCommandMessage>(
        [&](const ExternalCommandMessage& message,
            const SocketAddress& sourceAddress)
        {
          std::optional<AtlasNetClientID> cachedClientID =
              clientToAddressCache_.GetBySecond(sourceAddress);
          if (!cachedClientID)
          {
            logger->error("Received message from unknown address: {}, cannot "
                          "find associated ClientID",
                          sourceAddress.to_string());
            assert(false && "Received message from unknown address, cannot "
                            "find associated ClientID");
            return;
          }

          HandleExternalCommand(message, *cachedClientID);
        });
  }
  std::optional<AtlasNetGatewayID>
  GetManagingGateway(const AtlasNetClientID& clientID)
  {
  }
  std::optional<AtlasNetGatewayID>
  GetManagingGateway(const SocketAddress& address)
  {
  }

  /*Sets up that a given client's commands are forwarded to a set shard*/

  /*Sets up that this gateway manages a set client*/
  void DeclareGatewayRelay(const AtlasNetClientID& clientID);

private:
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("GatewayRelay");
  const Config config_;

  const std::string GatewayRelayKeyPrefix =
      Env::DatabaseNamespace + "gateway_relay{ATLASNET_GATEWAY_RELAY}:";
  const std::string ClientID2GatewayID =
      GatewayRelayKeyPrefix + "ClientID->GatewayRelay";
  const std::string GatewayID2ClientIDs_Set =
      GatewayRelayKeyPrefix + "GatewayID->ClientIDs";

  CacheMap<AtlasNetClientID, AtlasNetShardID> clientToShardCache_;

  CacheBiMap<AtlasNetClientID, SocketAddress> clientToAddressCache_;

  std::string
  GatewayID2ClientIDs_SetKey(const AtlasNetGatewayID& gatewayID) const
  {
    return GatewayID2ClientIDs_Set + ":" + gatewayID.to_string();
  }
  void HandleExternalCommand(const ExternalCommandMessage& message,
                             const AtlasNetClientID& sourceClientID)
  {
    logger->info("Received external command: {} from ClientID: {}",
                 message.envelope.commandName, sourceClientID.to_string());

    // Deserialize the command name and payload
    const std::string_view commandName = message.envelope.commandName;

    // Here you can implement logic to forward the command to the appropriate
    // shard or handle it as needed.
  }

  std::optional<SocketAddress>
  __GetClientAddressByID(const AtlasNetClientID& clientID)
  {
    return config_.clientRegistry->GetClientIDAddress(clientID);
  }
  std::optional<AtlasNetClientID>
  __GetClientIDByAddress(const SocketAddress& address)
  {
    return config_.clientRegistry->GetAddressClientID(address);
  }

  std::optional<AtlasNetShardID>
  __GetShardByClientID(const AtlasNetClientID& clientID)
  {
    return std::nullopt;
  }
};
} // namespace AtlasNet