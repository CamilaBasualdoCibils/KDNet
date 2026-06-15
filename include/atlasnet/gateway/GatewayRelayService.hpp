#pragma once

#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "enviroment/Enviroment.hpp"
#include <cassert>
namespace AtlasNet
{
class GatewayRelayService
{

public:
  struct Config
  {
    Database::RedisConn* redisConn;
    IService* containerService;
    // Add any necessary configuration parameters here
  };
  GatewayRelayService(const Config& config) : config_(config)
  {
    assert(config_.redisConn && "RedisConn pointer cannot be null");
    assert(config_.containerService &&
           "Container service pointer cannot be null");
  }
  std::optional<ServiceID> GetManagingGateway(const ClientID& clientID) {}
  std::optional<ServiceID> GetManagingGateway(const SocketAddress& address) {}
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
      bool success = config_.redisConn->Set().Modify().SRem(
          GatewayID2ClientIDs_SSetKey(existingGatewayIDParsed),
          clientIdWriter.as_string_view());
      assert(
          success &&
          "Failed to remove client from previous gateway's sorted set in Redis");
      if (Env::DebugMode)
      {
        bool debugSuccess = config_.redisConn->Set().Modify().SRem(
            GatewayID2ClientIDs_SSetKey(existingGatewayIDParsed) + ":debug",
            clientID.to_string());
        assert(
            debugSuccess &&
            "Failed to remove client from debug gateway relay mapping in Redis");
      }
    }
    // Set the new gateway for the client
    bool success = config_.redisConn->HashMap().GetSet().HSet(
        ClientID2GatewayID, clientIdWriter.as_string_view(),
        gatewayIdWriter.as_string_view());
    assert(success && "Failed to set gateway relay mapping in Redis");
    // Add the client to the set of clients for the new gateway
    success = config_.redisConn->Set().Modify().SAdd(
        GatewayID2ClientIDs_SSetKey(config_.containerService->GetID()),
        clientIdWriter.as_string_view());
    assert(success && "Failed to add client to gateway's set in Redis");
    if (Env::DebugMode)
    {
      bool debugSuccess = config_.redisConn->HashMap().GetSet().HSet(
          ClientID2GatewayID + ":debug", clientID.to_string(),
          config_.containerService->GetID().to_string());
      assert(debugSuccess &&
             "Failed to set debug gateway relay mapping in Redis");
      debugSuccess = config_.redisConn->Set().Modify().SAdd(
          GatewayID2ClientIDs_SSetKey(config_.containerService->GetID()) +
              ":debug",
          clientID.to_string());
      assert(debugSuccess &&
             "Failed to set debug gateway relay mapping in Redis");
    }
    std::cerr << "Declared gateway relay: ClientID " << clientID.to_string()
              << " is now managed by GatewayID "
              << config_.containerService->GetID().to_string() << std::endl;
  }

private:
  const Config config_;

  const std::string GatewayRelayKeyPrefix =
      Env::DatabaseNamespace + "gateway_relay{ATLASNET_PROXY_RELAY}:";
  const std::string ClientID2GatewayID =
      GatewayRelayKeyPrefix + "ClientID->GatewayRelay";
  const std::string GatewayID2ClientIDs_SSet =
      GatewayRelayKeyPrefix + "GatewayID->ClientIDs";

  std::string GatewayID2ClientIDs_SSetKey(const ServiceID& gatewayID) const
  {

    return GatewayID2ClientIDs_SSet + ":" + gatewayID.to_string();
  }
};
} // namespace AtlasNet