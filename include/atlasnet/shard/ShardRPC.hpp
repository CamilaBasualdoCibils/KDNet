#pragma once

#include "atlasnet/core/RPC/RPCMacros.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/entity/Entity.hpp"
namespace AtlasNet
{

struct ShardSpawnClientRequest
{
  Entity::Transform spawnTransform;
  ClientID clientID;
  ServiceID gatewayRelayID;
  std::vector<uint8_t>
      ClientLoginPayload; // This contains developer-defined data that was given
                          // by the login services that is specific to this
                          // client

  void Serialize(ByteWriter& writer) const
  {
    spawnTransform.Serialize(writer);
    writer.uuid(clientID);
    writer.uuid(gatewayRelayID);
    writer.blob(std::span<const uint8_t>(ClientLoginPayload.data(),
                                         ClientLoginPayload.size()));
  }

  void Deserialize(ByteReader& reader)
  {
    spawnTransform.Deserialize(reader);
    reader.uuid(clientID);
    reader.uuid(gatewayRelayID);
    std::span<const uint8_t> payloadSpan;
    reader.blob(payloadSpan);
    ClientLoginPayload =
        std::vector<uint8_t>(payloadSpan.begin(), payloadSpan.end());
  }
};
struct ShardSpawnClientResponse
{
  EntityID entityID;
  void Serialize(ByteWriter& writer) const
  {

    writer.uuid(entityID);
  }
  void Deserialize(ByteReader& reader)
  {
    reader.uuid(entityID);
  }
};
ATLASNET_RPC(ShardRPC,
             ATLASNET_RPC_METHOD(SpawnClient,
                                 ATLASNET_RPC_SIG(ShardSpawnClientResponse(
                                     ShardSpawnClientRequest))));
}; // namespace AtlasNet