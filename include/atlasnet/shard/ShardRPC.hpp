#pragma once

#include "atlasnet/core/RPC/RPCMacros.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
namespace AtlasNet
{
// Received by the backend of the shard
struct ShardSpawnClientRequest
{
  Entity::Position spawnTransform;
  AtlasNetClientID clientID;
  AtlasNetGatewayID gatewayRelayID;
  std::vector<uint8_t>
      clientSpawnPayload; // This contains developer-defined data that was given
                          // by the login services that is specific to this
                          // client

  void Serialize(ByteWriter& writer) const
  {
    spawnTransform.Serialize(writer);
    writer(clientID);
    writer(gatewayRelayID);
    writer.blob(std::span<const uint8_t>(clientSpawnPayload.data(),
                                         clientSpawnPayload.size()));
  }

  void Deserialize(ByteReader& reader)
  {
    spawnTransform.Deserialize(reader);
    reader(clientID);
    reader(gatewayRelayID);
    std::span<const uint8_t> payloadSpan;
    reader.blob(payloadSpan);
    clientSpawnPayload =
        std::vector<uint8_t>(payloadSpan.begin(), payloadSpan.end());
  }
};
// Received by the frontend of the shard
struct ClientSpawnInfo
{
  AtlasNetClientID clientID;
  AtlasNetEntityID entityID;
  Entity::Position position;
  std::vector<uint8_t>
      clientSpawnPayload; // This contains developer-defined data that will be
                          // given to the shard that spawns the client
};
struct ShardSpawnClientResponse
{
  AtlasNetEntityID entityID;
  void Serialize(ByteWriter& writer) const
  {

    writer(entityID);
  }
  void Deserialize(ByteReader& reader)
  {
    reader(entityID);
  }
};
ATLASNET_RPC(ShardRPC,
             ATLASNET_RPC_METHOD(SpawnClient,
                                 ATLASNET_RPC_SIG(ShardSpawnClientResponse(
                                     ShardSpawnClientRequest))));
}; // namespace AtlasNet