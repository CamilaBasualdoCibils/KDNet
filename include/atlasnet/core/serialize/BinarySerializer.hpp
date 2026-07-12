#pragma once

#include "bitsery/deserializer.h"
#include "bitsery/serializer.h"
#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <bitsery/brief_syntax.h>
#include <bitsery/traits/string.h>


#include <boost/container/small_vector.hpp>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
namespace AtlasNet
{

class BinaryDeserializer
{

public:
  using Data = const uint8_t*;
  using InputAdapter = bitsery::InputBufferAdapter<Data>;
  using Deserializer = bitsery::Deserializer<InputAdapter>;

  BinaryDeserializer(Data data, size_t size)
      : deserializer(InputAdapter{data, size})
  {
  }
  BinaryDeserializer(std::span<const uint8_t> bytes)
      : deserializer(InputAdapter{bytes.data(), bytes.size()})
  {
  }
  template <typename... Args> void Deserialize(Args&&... args)
  {
    (DeserializeOne(std::forward<Args>(args)), ...);
  }
  template <typename... Args> BinaryDeserializer& operator()(Args&&... args)
  {
    Deserialize(std::forward<Args>(args)...);
    return *this;
  }
  Deserializer* operator->()
  {
    return &deserializer;
  }

private:
  template <typename T> void DeserializeOne(T&& value)
  {
    deserializer(value);
  }
  Deserializer deserializer;
};
class BinarySerializer
{
public:
  using Buffer = std::vector<uint8_t>;
  using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
  using Serializer = bitsery::Serializer<OutputAdapter>;

  BinarySerializer() : buffer(), serializer(buffer) {}
  template <typename... Args> void Serialize(Args&&... args)
  {
    (SerializeOne(std::forward<Args>(args)), ...);
    serializer.adapter().flush();
  }
  template <typename... Args> BinarySerializer& operator()(Args&&... args)
  {
    Serialize(std::forward<Args>(args)...);
    return *this;
  }

  Serializer* operator->()
  {
    return &serializer;
  }
  void Flush()
  {
    serializer.adapter().flush();
  }
  auto GetBytes()
  {
    Flush();
    return std::span<const uint8_t>(buffer.data(),
                                    serializer.adapter().writtenBytesCount());
  }

private:
  template <typename T> void SerializeOne(T&& value)
  {
    serializer(value);
  }
  Buffer buffer;
  Serializer serializer;
};

struct XMLAdapter
{
};
struct JSONAdapter
{
};

} // namespace AtlasNet