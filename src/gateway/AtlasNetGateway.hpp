#pragma once

#include "atlasnet/controller/ControllerRPC.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/events/MessagingEvents.hpp"
#include "atlasnet/core/login/LoginEntry.hpp"
#include "atlasnet/core/login/LoginService.hpp"
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

    loginService_.emplace(LoginService::Config{
        ._globalEventSystem = &GetGlobalEventSystem(),
        .__redisConn = &GetRedisConn(),
        .containerService = this,
    });
    gatewayRelayService_.emplace(GatewayRelayService::Config{
        .redisConn = &GetRedisConn(),
        .containerService = this,
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
            std::cerr << "Internal connection established with address "
                      << event.address.to_string() << std::endl;
          }
          else
          {
            std::cerr
                << "Connection established with unknown source from address "
                << event.address.to_string() << std::endl;
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
      std::cerr << "Received handshake from client at "
                << remoteAddr.to_string() << std::endl;
      return HandshakeResponsePacket{.accepted = true};
    }
    else
    {
      return IService::HandleHandshake(identity, remoteAddr);
    }
  }
  void OnClientConnected(const ConnectionEstablishedEvent& event)
  {
    std::cerr << "Gateway detected new client connection established: "
              << event.address.to_string() << std::endl;

    std::cerr << "Logging in new client at " << event.address.to_string()
              << std::endl;
    const std::optional<LoginService::LoginResult> entry =
        loginService_->LoginClient(event.address);
    if (!entry)
      return;

    std::cerr << "Declaring gateway relay for ClientID: "
              << entry->clientID.to_string() << std::endl;
    gatewayRelayService_->DeclareGatewayRelay(entry->clientID);

    // Eventually this will be implemented
    /* auto shardID_future =
        GetRPCSystem().Call<ControllerRPC::GetClosestShardToLocation>(
            GetControllerAddress(), entry->SpawnLocation);

    shardID_future.wait_for(std::chrono::seconds(5));
    if (shardID_future.valid())
    {
      const ShardID shardID = shardID_future.get();
      std::cerr << "Received closest shard ID " << shardID.to_string()
                << " for client " << entry->clientID.to_string()
                << std::endl;
    }
    else
    {
      std::cerr << "Failed to receive closest shard ID for client "
                << entry->clientID.to_string() << " within timeout."
                << std::endl;
    } */
    std::vector<ServiceRegistry::ServiceInfo> services;
    GetServiceRegistry().GetServicesOfType(ServiceType::Shard, services);

    if (services.empty())
      throw std::runtime_error("No shard services found in registry");

    std::cerr << "Shard at " << services[0].address.to_string() << " with ID "
              << services[0].id.to_string() << std::endl;

    ShardSpawnClientRequest request{
        .spawnTransform = entry->SpawnLocation.transform,
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

      std::cerr << "Successfully spawned client entity with ID "
                << spawnResponse->entityID.to_string() << " on shard "
                << services[0].id.to_string() << std::endl;
    }
    else
    {
      std::cerr << "Failed to receive spawn result for client "
                << entry->clientID.to_string() << " within timeout."
                << std::endl;
    }
  }
  std::optional<LoginService> loginService_;
  std::optional<GatewayRelayService> gatewayRelayService_;
};
} // namespace AtlasNet