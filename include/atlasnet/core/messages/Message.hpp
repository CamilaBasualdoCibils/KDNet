#pragma once

#include <functional>
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/MacroConcepts.hpp"

namespace AtlasNet
{
using MessageIDHash = std::size_t;

class IMessage
{
public:
  static MessageIDHash DeserializeTypeIdHash(ByteReader& archive)
  {
    MessageIDHash typeIdHash;
    archive(typeIdHash);
    return typeIdHash;
  }
};

} // namespace AtlasNet

// =====================================================
// user-facing field syntax
// =====================================================
#define ATLASNET_MESSAGE_DATA(Type, Name) (Type, Name)


// =====================================================
// unwrap (Type, Name)
// =====================================================

#define ATLASNET_DECLARE_FIELD(Field) ATLASNET_DECLARE_FIELD_I Field
#define ATLASNET_DECLARE_FIELD_I(Type, Name) Type Name;

#define ATLASNET_SERIALIZE_FIELD(Field) ATLASNET_SERIALIZE_FIELD_I Field
#define ATLASNET_SERIALIZE_FIELD_I(Type, Name) archive(Name);


// =====================================================
// hash name (unchanged logic, but FIXED consistency issue)
// =====================================================

#define ATLASNET_HASH_NAME(Name)                                               \
const static inline std::string GetName() { return #Name; } \
  const static inline AtlasNet::MessageIDHash TypeIdHash =                     \
      std::hash<std::string_view>{}(GetName());


// =====================================================
// final message macro (UPDATED to new macro system)
// =====================================================

#define ATLASNET_MESSAGE(Name, ...)                                            \
  struct Name : AtlasNet::IMessage                                             \
  {                                                                            \
    ATLASNET_FOR_EACH(ATLASNET_DECLARE_FIELD, ATLASNET_SEP_NONE, __VA_ARGS__) \
                                                                               \
    ATLASNET_HASH_NAME(Name);                                                  \
                                                                               \
    void Serialize(AtlasNet::ByteWriter& archive) const                        \
    {                                                                          \
      archive(TypeIdHash);                                                     \
      ATLASNET_FOR_EACH(ATLASNET_SERIALIZE_FIELD, ATLASNET_SEP_NONE, __VA_ARGS__) \
    }                                                                          \
                                                                               \
    void Deserialize(AtlasNet::ByteReader& archive)                            \
    {                                                                          \
      AtlasNet::MessageIDHash typeIdHash =                                     \
          AtlasNet::IMessage::DeserializeTypeIdHash(archive);                  \
      (void)typeIdHash;                                                        \
                                                                               \
      ATLASNET_FOR_EACH(ATLASNET_SERIALIZE_FIELD, ATLASNET_SEP_NONE, __VA_ARGS__) \
    }                                                                          \
  };