#pragma once

#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include <boost/describe.hpp>
#include <charconv>
#include <cstdint>
#include <format>
#include <fstream>
#include <system_error>
namespace AtlasNet
{

enum class AtlasNetNodeType : uint8_t
{
  Controller = 0,
  Shard = 1,
  Gateway = 2,
  Cartograph = 3,
  Invalid = 4
};
BOOST_DESCRIBE_ENUM(AtlasNetNodeType, Controller, Shard, Gateway, Cartograph,
                    Invalid);

template <typename underlying_type, typename tag> struct StrongTypedef
{
  using underlying_type_t = underlying_type;
  explicit StrongTypedef(underlying_type value) : value(value) {}
  StrongTypedef(const StrongTypedef&) = default;
  StrongTypedef& operator=(const StrongTypedef&) = default;
  StrongTypedef() : value() {}
  underlying_type value;

  operator underlying_type() const
  {
    return value;
  }
  void Serialize(ByteWriter& writer) const
  {
    writer(value);
  }
  void Deserialize(ByteReader& reader)
  {
    reader(value);
  }
  std::string to_string() const
  {
    return std::to_string(value);
  }
  static StrongTypedef from_string(const std::string_view& str)
  {
    StrongTypedef obj(underlying_type{});
    auto [ptr, ec] =
        std::from_chars(str.data(), str.data() + str.size(), obj.value);
    if (ec != std::errc{})
    {
      throw std::runtime_error("Failed to convert string to StrongTypedef");
    }
    return obj;
  }

  std::ofstream& operator<<(std::ofstream& os) const
  {
    os << value;
    return os;
  }
};
using AtlasNetNodeID = StrongTypedef<uint32_t, struct NodeIDTag>;
using AtlasNetShardID = StrongTypedef<uint32_t, struct ShardIDTag>;
using AtlasNetGatewayID = StrongTypedef<uint32_t, struct GatewayIDTag>;
using AtlasNetControllerID = StrongTypedef<uint32_t, struct ControllerIDTag>;
struct GatewayNodeInfo
{
  AtlasNetGatewayID id;
  void Serialize(ByteWriter& bw) const
  {
    bw(id);
  }
  void Deserialize(ByteReader& br)
  {
    br(id);
  }
  void to_json(_Json& j) const
  {
    j = _Json{{"id", id.to_string()}};
  }
};
struct ShardNodeInfo
{
  AtlasNetShardID id;

  void Serialize(ByteWriter& bw) const
  {
    bw(id);
  }
  void Deserialize(ByteReader& br)
  {
    br(id);
  }
  void to_json(_Json& j) const
  {
    j = _Json{{"id", id.to_string()}};
  }
};
struct ControllerNodeInfo
{
  AtlasNetControllerID id;
  void Serialize(ByteWriter& bw) const
  {
    bw(id);
  }
  void Deserialize(ByteReader& br)
  {
    br(id);
  }
  void to_json(_Json& j) const
  {
    j = _Json{{"id", id.to_string()}};
  }
};
struct CartographBackendInfo
{

  void Serialize(ByteWriter& bw) const {}
  void Deserialize(ByteReader& br)
  {
    // Deserialize any relevant information for the CartographBackendInfo
  }
  void to_json(_Json& j) const
  {
    j = _Json{};
  }
};
struct NodeInfo
{
  AtlasNetNodeID id;
  SocketAddress address;
  AtlasNetNodeType containerType;
  std::optional<std::variant<GatewayNodeInfo, ShardNodeInfo, ControllerNodeInfo,
                             CartographBackendInfo>>
      specificInfo;
  void Serialize(ByteWriter& bw) const
  {
    bw(id);
    bw(address);
    bw(containerType);
    // Serialize the specificInfo variant
    if (specificInfo.has_value())
    {
      std::visit([&bw](auto&& arg) { arg.Serialize(bw); }, *specificInfo);
    }
  }
  void Deserialize(ByteReader& br)
  {
    br(id);
    br(address);
    br(containerType);
    // Deserialize the specificInfo variant
    switch (containerType)
    {
    case AtlasNetNodeType::Gateway:
      specificInfo = GatewayNodeInfo{};
      break;
    case AtlasNetNodeType::Shard:
      specificInfo = ShardNodeInfo{};
      break;
    case AtlasNetNodeType::Controller:
      specificInfo = ControllerNodeInfo{};
      break;
    case AtlasNetNodeType::Cartograph:
      specificInfo = CartographBackendInfo{};
      break;
    default:
      throw std::runtime_error("Unknown node type");
    }

    std::visit([&br](auto&& arg) { arg.Deserialize(br); }, *specificInfo);
  }
  void to_json(_Json& j) const
  {
    j = _Json{{"id", id.to_string()},
              {"address", address.to_string()},
              {"containerType",
               boost::describe::enum_to_string(containerType, "UNKNOWN")}};
    if (specificInfo.has_value())
    {
      std::visit(
          [&j](auto&& arg)
          {
            _Json specificJson;
            arg.to_json(specificJson);
            j["specificInfo"] = specificJson;
          },
          *specificInfo);
    }
  }
};
}; // namespace AtlasNet
namespace std
{
template <typename underlying_type, typename tag>
struct hash<AtlasNet::StrongTypedef<underlying_type, tag>>
{
  size_t operator()(
      const AtlasNet::StrongTypedef<underlying_type, tag>& value) const noexcept
  {
    return std::hash<underlying_type>{}(value.value);
  }
};

 template <typename Underlying, typename Tag>
    struct formatter<AtlasNet::StrongTypedef<Underlying, Tag>>
        : formatter<Underlying>
    {
        template <typename FormatContext>
        auto format(const AtlasNet::StrongTypedef<Underlying, Tag>& value,
                    FormatContext& ctx) const
        {
            return formatter<Underlying>::format(value.value, ctx);
        }
    };
} // namespace std