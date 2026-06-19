#pragma once
#include "atlasnet/core/messages/Message.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/shard/shard.hpp"
#include "boost-src/libs/container/include/boost/container/small_vector.hpp"
#include "boost-src/libs/static_string/include/boost/static_string/static_string.hpp"
#include <atlasnet/core/entity/Entity.hpp>
#include <cstdint>
#include <string>
namespace AtlasNet
{
struct CommandEnvelope
{
  const static size_t MaxCommandNameLength = 64;
  boost::static_string<MaxCommandNameLength> commandName;

  EntityID targetEntity;
  uint64_t logical_entity_sequence;

  enum SenderType : uint8_t
  {
    Shard = 0,
    Client = 1
  } senderType;
  UUID sender;

  void Serialize(ByteWriter& serializer) const
  {
    serializer.blob(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(commandName.data()),
        commandName.size()));
    serializer.uuid(targetEntity);
    serializer.u64(logical_entity_sequence);
    serializer.u8(static_cast<uint8_t>(senderType));
    serializer.uuid(sender);
  }

  void Deserialize(ByteReader& deserializer)
  {
    std::span<const uint8_t> commandNameSpan;
    deserializer.blob(commandNameSpan);
    commandName.assign(reinterpret_cast<const char*>(commandNameSpan.data()),
                       commandNameSpan.size());
    deserializer.uuid(targetEntity);
    deserializer.u64(logical_entity_sequence);
    deserializer.u8(reinterpret_cast<uint8_t&>(senderType));
    deserializer.uuid(sender);
  }
};

struct PayloadCommand
{
  CommandEnvelope envelope;
  boost::container::small_vector<uint8_t, 64> payload;

  void Serialize(ByteWriter& serializer) const
  {
    envelope.Serialize(serializer);
    serializer.write(payload.data(), payload.size());
  }
  void Deserialize(ByteReader& deserializer)
  {
    envelope.Deserialize(deserializer);
    payload.resize(deserializer.remaining());
    deserializer.read(payload.data(), payload.size());
  }
};

ATLASNET_MESSAGE(CommandMessage,
                 ATLASNET_MESSAGE_DATA(CommandEnvelope, envelope),
                 ATLASNET_MESSAGE_DATA(PayloadCommand, payload));
} // namespace AtlasNet
