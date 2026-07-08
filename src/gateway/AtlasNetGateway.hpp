#pragma once

#include "atlasnet/client/ClientRPC.hpp"
#include "atlasnet/controller/ControllerRPC.hpp"
#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/client/ClientRegistry.hpp"

#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/events/MessagingEvents.hpp"
#include "atlasnet/core/messages/HandshakePacket.hpp"
#include "atlasnet/core/node/AtlasNetNode.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
#include "atlasnet/gateway/GatewayRelayService.hpp"
#include "atlasnet/shard/ShardRPC.hpp"
#include <chrono>
#include <future>
#include <optional>
#include <vector>

namespace AtlasNet
{
class AtlasNetGateway : public IAtlasNetNode
{
public:
  AtlasNetGateway() : IAtlasNetNode(AtlasNetNodeType::Gateway) {}
  ~AtlasNetGateway() override = default;

  AtlasNetGatewayID GetGatewayID() const
  {
    return AtlasNetGatewayID(0);
  }

private:
  void OnInit() override
  {
    clientIDGenerator.emplace(GetNodeID());
    clientRegistry.emplace(ClientRegistry::Config{
        ._globalEventSystem = &GetGlobalEventSystem(),
        .__redisConn = &GetRedisConn(),
        ._clientIDGenerator = &*clientIDGenerator,
    });
    gatewayRelayService_.emplace(GatewayRelayService::Config{
        .redisConn = &GetRedisConn(),
        .gateway = this,
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
            GetLogger()->warn(
                "Connection established with unknown source from address {}",
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
      GetLogger()->info("Received handshake from client at {}",
                        remoteAddr.to_string());
      return HandshakeResponsePacket{.accepted = true};
    }
    else
    {
      return IAtlasNetNode::HandleHandshake(identity, remoteAddr);
    }
  }
  void OnClientConnected(const ConnectionEstablishedEvent& event);
  std::optional<ClientRegistry> clientRegistry;
  std::optional<GatewayRelayService> gatewayRelayService_;
  std::optional<AtlasNetClientID::Generator> clientIDGenerator;
};
} // namespace AtlasNet