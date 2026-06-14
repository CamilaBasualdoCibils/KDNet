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
  ServiceID proxyRelayID;
  std::vector<uint8_t>
      ClientLoginPayload; // This contains developer-defined data that was given
                          // by the login services that is specific to this
                          // client

  void Serialize(ByteWriter& writer) const
  {
    spawnTransform.Serialize(writer);
    writer.uuid(clientID);
    writer.uuid(proxyRelayID);
    writer.blob(std::span<const uint8_t>(ClientLoginPayload.data(),
                                         ClientLoginPayload.size()));
  }

  void Deserialize(ByteReader& reader)
  {
    spawnTransform.Deserialize(reader);
    reader.uuid(clientID);
    reader.uuid(proxyRelayID);
    std::span<const uint8_t> payloadSpan;
    reader.blob(payloadSpan);
    ClientLoginPayload =
        std::vector<uint8_t>(payloadSpan.begin(), payloadSpan.end());
  }
};
struct ShardSpawnClientResponse
{
  std::optional<EntityID> entityID;
  void Serialize(ByteWriter& writer) const
  {
    if (entityID.has_value())
    {
      writer.u8(true);
      writer.uuid(entityID.value());
    }
    else
    {
      writer.u8(false);
    }
  }
  void Deserialize(ByteReader& reader)
  {
    uint8_t hasEntityID_v;
    reader.u8(hasEntityID_v);
    bool hasEntityID = hasEntityID_v != 0;
    if (hasEntityID)    {
      EntityID id;
      reader.uuid(id);
      entityID = id;
    }
    else    {
      entityID = std::nullopt;
    }
  }
};
ATLASNET_RPC(ShardRPC,
             ATLASNET_RPC_METHOD(SpawnClient,
                                 ATLASNET_RPC_SIG(ShardSpawnClientResponse(
                                     ShardSpawnClientRequest))));
}; // namespace AtlasNet