#pragma once

#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/client/ClientRegistry.hpp"

#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/command/Command.hpp"
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
  GatewayRelayService(const Config& config) : config_(config)
  {
    assert(config_.redisConn && "RedisConn pointer cannot be null");
    assert(config_.gateway && "Gateway pointer cannot be null");
    assert(config_.messageSystem && "MessageSystem pointer cannot be null");
    assert(config_.clientRegistry && "ClientRegistry pointer cannot be null");

    config_.messageSystem->On<ExternalCommandMessage>(
        [&](const ExternalCommandMessage& message,
            const SocketAddress& sourceAddress)
        {
          AtlasNetClientID sourceClientID;
          {
            std::shared_lock lock(cacheMutex_);
            const auto it = addressToClientCache_.find(sourceAddress);
            // if it does not exist add to cache.
            if (it == addressToClientCache_.end())
            {
              std::optional<AtlasNetClientID> clientID =
                  config_.clientRegistry->GetAddressClientID(sourceAddress);
              assert(clientID &&
                     "we received an external command from a client without a "
                     "registered ClientID. This should not happen.");

              lock.unlock();
              std::unique_lock uniqueLock(cacheMutex_);
              addressToClientCache_[sourceAddress] = *clientID;
              sourceClientID = *clientID;
            }
            else
            {
              sourceClientID = it->second;
            }
          }

          HandleExternalCommand(message, sourceClientID);
        });
  }
  std::optional<AtlasNetGatewayID> GetManagingGateway(const AtlasNetClientID& clientID)
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
  std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("GatewayRelay");
  const Config config_;

  const std::string GatewayRelayKeyPrefix =
      Env::DatabaseNamespace + "gateway_relay{ATLASNET_GATEWAY_RELAY}:";
  const std::string ClientID2GatewayID =
      GatewayRelayKeyPrefix + "ClientID->GatewayRelay";
  const std::string GatewayID2ClientIDs_Set =
      GatewayRelayKeyPrefix + "GatewayID->ClientIDs";

  std::shared_mutex cacheMutex_;
  std::unordered_map<AtlasNetClientID, AtlasNetShardID> clientToShardCache_;
  // std::unordered_map<ClientID, SocketAddress> clientToAddressCache_;
  std::unordered_map<SocketAddress, AtlasNetClientID> addressToClientCache_;

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

    std::shared_lock lock(cacheMutex_);
    if (const auto it = clientToShardCache_.find(sourceClientID);
        it == clientToShardCache_.end())
    {
    }

    // Here you can implement logic to forward the command to the appropriate
    // shard or handle it as needed.
  }
};
} // namespace AtlasNet