#pragma once

#include "atlasnet/client/ClientRPC.hpp"
#include "atlasnet/controller/ControllerRPC.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/client/ClientRegistry.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/events/MessagingEvents.hpp"
#include "atlasnet/core/messages/HandshakePacket.hpp"
#include "atlasnet/core/service/ServiceRegistry.hpp"
#include "atlasnet/gateway/GatewayRelayService.hpp"
#include "atlasnet/shard/ShardRPC.hpp"
#include <chrono>
#include <future>
#include <optional>
#include <vector>

namespace AtlasNet
{
class AtlasNetGateway : public IService
{
public:
  AtlasNetGateway() : IService(ServiceType::Gateway) {}
  ~AtlasNetGateway() override = default;

private:
  void OnInit() override
  {

    clientRegistry.emplace(ClientRegistry::Config{
        ._globalEventSystem = &GetGlobalEventSystem(),
        .__redisConn = &GetRedisConn(),
        .containerService = this,
    });
    gatewayRelayService_.emplace(GatewayRelayService::Config{
        .redisConn = &GetRedisConn(),
        .containerService = this,
        .messageSystem = &GetMessageSystem(),
        .clientRegistry = &*clientRegistry,
    });
    GetMessageSystem().OpenListenSocket(Env::GatewayListenPort);
    GetLocalEventSystem().On<ConnectionEstablishedEvent>(
        [&](const ConnectionEstablishedEvent& event)
        {
          if (event.source == ConnectionSource::External)
          {
            OnClientConnected(event);
          }
          else if (event.source == ConnectionSource::Internal)
          {
            GetLogger()->info("Internal connection established with address {}",
                           event.address.to_string());
  
          }
          else
          {
            GetLogger()->warn("Connection established with unknown source from address {}",
                         event.address.to_string());
          }
        });
  }
  void OnShutdown() override {}

  HandshakeResponsePacket
  HandleHandshake(const HandshakeIdentity& identity,
                  const SocketAddress& remoteAddr) override
  {
    if (identity.role == HandshakeRole::eClient)
    {
      GetLogger()->info("Received handshake from client at {}", remoteAddr.to_string());
      return HandshakeResponsePacket{.accepted = true};
    }
    else
    {
      return IService::HandleHandshake(identity, remoteAddr);
    }
  }
  void OnClientConnected(const ConnectionEstablishedEvent& event)
  {
    GetLogger()->info("Gateway detected new client connection established: {}", event.address.to_string());

    GetLogger()->info("Logging in new client at {}", event.address.to_string());
    const std::optional<ClientRegistry::LoginResult> entry =
        clientRegistry->LoginClient(event.address);
    if (!entry)
      return;

    GetLogger()->info("Declaring gateway relay for ClientID: {}", entry->clientID.to_string());
    gatewayRelayService_->DeclareGatewayRelay(entry->clientID);

    // Eventually this will be implemented
    /* auto shardID_future =
        GetRPCSystem().Call<ControllerRPC::GetClosestShardToLocation>(
            GetControllerAddress(), entry->SpawnLocation);

    shardID_future.wait_for(std::chrono::seconds(5));
    if (shardID_future.valid())
    {
      const ShardID shardID = shardID_future.get();
      logger->info("Received closest shard ID {} for client {}",
                   shardID.to_string(), entry->clientID.to_string());

    }
    else
    {
    logger->error("Failed to receive closest shard ID for client {} within timeout.",
                 entry->clientID.to_string());

    } */
    std::vector<PresenceService::ServiceInfo> services;
    GetServiceRegistry().GetServicesOfType(ServiceType::Shard, services);

    if (services.empty())
      throw std::runtime_error("No shard services found in registry");

    GetLogger()->info("Shard at {} with ID {}", services[0].address.to_string(), services[0].id.to_string());

    ShardSpawnClientRequest request{
        .spawnTransform = entry->SpawnLocation.position,
        .clientID = entry->clientID,
        .gatewayRelayID = GetID(),
    };

    auto spawnResult = GetRPCSystem().Call<ShardRPC::SpawnClient>(
        SocketAddress(services[0].address, Env::InternalMessagePort), request);

    std::future_status status = spawnResult.wait_for(std::chrono::seconds(5));
    std::optional<ShardSpawnClientResponse> spawnResponse;
    if (status == std::future_status::ready)
      spawnResponse = spawnResult.get();

    if (spawnResponse)
    {

      GetLogger()->info("Successfully spawned client entity with ID {} on shard {}",
                        spawnResponse->entityID.to_string(), services[0].id.to_string());

      gatewayRelayService_->DeclareGatewayRelay(entry->clientID);
      clientRegistry->AssociateClientWithEntity(entry->clientID,
                                                spawnResponse->entityID);

      ClientConnectionCompleteData result;
      result.clientID = entry->clientID;
      result.entityID = spawnResponse->entityID;
      result.result = ClientConnectionResult::Success;

      GetRPCSystem().Call<ClientRPC::ClientConnectionCompleteNotification>(
          event.address, result);
    }
    else
    {
      GetLogger()->error("Failed to receive spawn result for client {} within timeout.",
                         entry->clientID.to_string());
    }
  }
  std::optional<ClientRegistry> clientRegistry;
  std::optional<GatewayRelayService> gatewayRelayService_;
};
} // namespace AtlasNet