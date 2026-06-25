#pragma once

#include "atlasnet/core/RPC/RPCMacros.hpp"
#include "atlasnet/core/entity/Entity.hpp"
namespace AtlasNet
{
enum class ClientConnectionResult : uint8_t
{
  Success = 0,
  Failure = 1
};
BOOST_DESCRIBE_ENUM(ClientConnectionResult, Success, Failure);
struct ClientConnectionCompleteData
{

  ClientConnectionResult result;
  ClientID clientID;
  EntityID entityID;

  void Serialize(ByteWriter& writer) const
  {
    writer.u8(static_cast<uint8_t>(result));
    writer.uuid(clientID);
    writer.uuid(entityID);
  }

  void Deserialize(ByteReader& reader)
  {
    uint8_t result_v;
    reader.u8(result_v);
    result = static_cast<ClientConnectionResult>(result_v);
    reader.uuid(clientID);
    reader.uuid(entityID);
  }
};
ATLASNET_RPC(
    ClientRPC,
    ATLASNET_RPC_METHOD(ClientConnectionCompleteNotification,
                        ATLASNET_RPC_SIG(void(ClientConnectionCompleteData))));
}; // namespace AtlasNet