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
class ProxyRelayService
{

public:
  struct Config
  {
    Database::RedisConn* redisConn;
    IService* containerService;
    // Add any necessary configuration parameters here
  };
  ProxyRelayService(const Config& config) : config_(config)
  {
    assert(config_.redisConn && "RedisConn pointer cannot be null");
    assert(config_.containerService &&
           "Container service pointer cannot be null");
  }
  std::optional<ServiceID> GetManagingProxy(const ClientID& clientID) {}
  std::optional<ServiceID> GetManagingProxy(const SocketAddress& address) {}
  void DeclareProxyRelay(const ClientID& clientID)
  {
    ByteWriter clientIdWriter;
    clientIdWriter.uuid(clientID);
    ByteWriter proxyIdWriter;
    proxyIdWriter.uuid(config_.containerService->GetID());

    // Check if the client already has an associated proxy and remove it from
    // the sorted set if it exists
    if (std::optional<std::string> existingProxyID =
            config_.redisConn->HashMap().GetSet().HGet(
                ClientID2ProxyID, clientIdWriter.as_string_view()))
    {
      ByteReader existingProxyIDReader(existingProxyID.value());
      ServiceID existingProxyIDParsed;
      existingProxyIDReader.uuid(existingProxyIDParsed);
      bool success = config_.redisConn->Set().Modify().SRem(
          ProxyID2ClientIDs_SSetKey(existingProxyIDParsed),
          clientIdWriter.as_string_view());
      assert(
          success &&
          "Failed to remove client from previous proxy's sorted set in Redis");
      if (Env::DebugMode)
      {
        bool debugSuccess = config_.redisConn->Set().Modify().SRem(
            ProxyID2ClientIDs_SSetKey(existingProxyIDParsed) + ":debug",
            clientID.to_string());
        assert(
            debugSuccess &&
            "Failed to remove client from debug proxy relay mapping in Redis");
      }
    }
    // Set the new proxy for the client
    bool success = config_.redisConn->HashMap().GetSet().HSet(
        ClientID2ProxyID, clientIdWriter.as_string_view(),
        proxyIdWriter.as_string_view());
    assert(success && "Failed to set proxy relay mapping in Redis");
    // Add the client to the set of clients for the new proxy
    success = config_.redisConn->Set().Modify().SAdd(
        ProxyID2ClientIDs_SSetKey(config_.containerService->GetID()),
        clientIdWriter.as_string_view());
    assert(success && "Failed to add client to proxy's set in Redis");
    if (Env::DebugMode)
    {
      bool debugSuccess = config_.redisConn->HashMap().GetSet().HSet(
          ClientID2ProxyID + ":debug", clientID.to_string(),
          config_.containerService->GetID().to_string());
      assert(debugSuccess &&
             "Failed to set debug proxy relay mapping in Redis");
      debugSuccess = config_.redisConn->Set().Modify().SAdd(
          ProxyID2ClientIDs_SSetKey(config_.containerService->GetID()) +
              ":debug",
          clientID.to_string());
      assert(debugSuccess &&
             "Failed to set debug proxy relay mapping in Redis");
    }
    std::cerr << "Declared proxy relay: ClientID " << clientID.to_string()
              << " is now managed by ProxyID "
              << config_.containerService->GetID().to_string() << std::endl;
  }

private:
  const Config config_;

  const std::string ProxyRelayKeyPrefix =
      Env::DatabaseNamespace + "proxy_relay{ATLASNET_PROXY_RELAY}:";
  const std::string ClientID2ProxyID =
      ProxyRelayKeyPrefix + "ClientID->ProxyRelay";
  const std::string ProxyID2ClientIDs_SSet =
      ProxyRelayKeyPrefix + "ProxyID->ClientIDs";

  std::string ProxyID2ClientIDs_SSetKey(const ServiceID& proxyID) const
  {

    return ProxyID2ClientIDs_SSet + ":" + proxyID.to_string();
  }
};
} // namespace AtlasNet