#pragma once

#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/client/ClientRegistry.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/command/Command.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/shard/shard.hpp"
#include "enviroment/Enviroment.hpp"
#include <cassert>
#include <shared_mutex>
namespace AtlasNet
{
class GatewayRelayService
{

public:
  struct Config
  {
    Database::RedisConn* redisConn;
    IService* containerService;
    MessageSystem* messageSystem;
    ClientRegistry* clientRegistry;
    // Add any necessary configuration parameters here
  };
  GatewayRelayService(const Config& config) : config_(config)
  {
    assert(config_.redisConn && "RedisConn pointer cannot be null");
    assert(config_.containerService &&
           "Container service pointer cannot be null");
    assert(config_.messageSystem && "MessageSystem pointer cannot be null");
    assert(config_.clientRegistry && "ClientRegistry pointer cannot be null");

    config_.messageSystem->On<ExternalCommandMessage>(
        [&](const ExternalCommandMessage& message,
            const SocketAddress& sourceAddress)
        {
          ClientID sourceClientID;
          {
            std::shared_lock lock(cacheMutex_);
            const auto it = addressToClientCache_.find(sourceAddress);
            // if it does not exist add to cache.
            if (it == addressToClientCache_.end())
            {
              std::optional<ClientID> clientID =
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
  std::optional<ServiceID> GetManagingGateway(const ClientID& clientID) {}
  std::optional<ServiceID> GetManagingGateway(const SocketAddress& address) {}

  /*Sets up that a given client's commands are forwarded to a set shard*/

  /*Sets up that this gateway manages a set client*/
  void DeclareGatewayRelay(const ClientID& clientID)
  {
    ByteWriter clientIdWriter;
    clientIdWriter.uuid(clientID);
    ByteWriter gatewayIdWriter;
    gatewayIdWriter.uuid(config_.containerService->GetID());

    // Check if the client already has an associated gateway and remove it from
    // the sorted set if it exists
    if (std::optional<std::string> existingGatewayID =
            config_.redisConn->HashMap().GetSet().HGet(
                ClientID2GatewayID, clientIdWriter.as_string_view()))
    {
      ByteReader existingGatewayIDReader(existingGatewayID.value());
      ServiceID existingGatewayIDParsed;
      existingGatewayIDReader.uuid(existingGatewayIDParsed);
      config_.redisConn->Set().Modify().SRem(
          GatewayID2ClientIDs_SetKey(existingGatewayIDParsed),
          clientIdWriter.as_string_view());

      if (Env::DebugMode)
      {
        config_.redisConn->Set().Modify().SRem(
            GatewayID2ClientIDs_SetKey(existingGatewayIDParsed) + "_debug",
            clientID.to_string());
      }
    }
    // Set the new gateway for the client
    config_.redisConn->HashMap().GetSet().HSet(
        ClientID2GatewayID, clientIdWriter.as_string_view(),
        gatewayIdWriter.as_string_view());

    // Add the client to the set of clients for the new gateway
    config_.redisConn->Set().Modify().SAdd(
        GatewayID2ClientIDs_SetKey(config_.containerService->GetID()),
        clientIdWriter.as_string_view());
    if (Env::DebugMode)
    {
      config_.redisConn->HashMap().GetSet().HSet(
          ClientID2GatewayID + "_debug", clientID.to_string(),
          config_.containerService->GetID().to_string());
      config_.redisConn->Set().Modify().SAdd(
          GatewayID2ClientIDs_SetKey(config_.containerService->GetID()) +
              "_debug",
          clientID.to_string());
    }
    logger->info("Declared gateway relay: ClientID {} is now managed by GatewayID {}",
                 clientID.to_string(),
                 config_.containerService->GetID().to_string());
  }

private:
std::shared_ptr<spdlog::logger> logger = spdlog::get("GatewayRelay");
  const Config config_;

  const std::string GatewayRelayKeyPrefix =
      Env::DatabaseNamespace + "gateway_relay{ATLASNET_GATEWAY_RELAY}:";
  const std::string ClientID2GatewayID =
      GatewayRelayKeyPrefix + "ClientID->GatewayRelay";
  const std::string GatewayID2ClientIDs_Set =
      GatewayRelayKeyPrefix + "GatewayID->ClientIDs";

  std::shared_mutex cacheMutex_;
  std::unordered_map<ClientID, ShardID> clientToShardCache_;
  // std::unordered_map<ClientID, SocketAddress> clientToAddressCache_;
  std::unordered_map<SocketAddress, ClientID> addressToClientCache_;

  std::string GatewayID2ClientIDs_SetKey(const ServiceID& gatewayID) const
  {

    return GatewayID2ClientIDs_Set + ":" + gatewayID.to_string();
  }
  void HandleExternalCommand(const ExternalCommandMessage& message,
                             const ClientID& sourceClientID)
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