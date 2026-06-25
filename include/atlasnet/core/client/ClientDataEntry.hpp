#pragma once

#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/entity/Entity.hpp"

namespace AtlasNet
{
struct LoginData
{
  SocketAddress address;
  ServiceID managingGateway;
  ClientID clientID;
  std::optional<EntityID> entityID;

  void Serialize(ByteWriter& writer) const
  {
    address.Serialize(writer);
    writer.uuid(managingGateway);
    writer.uuid(clientID);
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
    address.Deserialize(reader);
    reader.uuid(managingGateway);
    reader.uuid(clientID);
    uint8_t hasEntityID_v;
    reader.u8(hasEntityID_v);
    bool hasEntityID = hasEntityID_v != 0;
    if (hasEntityID)
    {
      EntityID id;
      reader.uuid(id);
      entityID = id;
    }
    else
    {
      entityID = std::nullopt;
    }
  }

  void to_json(_Json& j) const
  {
    j = _Json{
        {"address", address.to_string()},
        {"managingGateway", managingGateway.to_string()},
        {"clientID", clientID.to_string()},
        {"entityID",
         entityID.has_value() ? entityID.value().to_string() :""},
    };
  }
};
}; // namespace AtlasNet