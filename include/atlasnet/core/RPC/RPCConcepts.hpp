#pragma once

#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/messages/Message.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/utils/MacroConcepts.hpp"
#include <cstdint>
#include <expected>
#include <span>
#include <type_traits>
#include <boost/describe.hpp>
namespace AtlasNet
{
using RPCID = uint32_t;
using RPCCallID = uint64_t;
struct RPCContext
{
  SocketAddress caller;
};

enum class RPCError
{
  None = 0,
  Timeout = 1,
  UnknownRPC = 2,
  InvalidPayload = 3,
  PermissionDenied = 4,
  RemoteException = 5,
};
BOOST_DESCRIBE_ENUM(RPCError, None, Timeout, UnknownRPC, InvalidPayload,
                    PermissionDenied, RemoteException);
template <typename T> using TRPCResult = std::expected<T, RPCError>;
using RPCResult = TRPCResult<std::vector<uint8_t>>;
struct RPCResultW
{
  RPCResult result;
  void Serialize(ByteWriter& ar) const
  {
    ar(result.has_value());
    if (result.has_value())
    {
      ar(result.value());
    }
    else
    {
      ar(static_cast<uint8_t>(result.error()));
    }
  }
  void Deserialize(ByteReader& ar)
  {
    bool hasValue;
    ar(hasValue);
    if (hasValue)
    {
      std::vector<uint8_t> value;
      ar(value);
      result = std::move(value);
    }
    else
    {
      uint8_t errorCode;
      ar(errorCode);
      result = std::unexpected(static_cast<RPCError>(errorCode));
    }
  }
};
using RPC_BindCallFunction_Raw =
    std::function<RPCResult(const RPCContext&, std::span<const uint8_t>)>;

namespace RPC_Internal
{

struct RPCRequestContext
{
  RPCID rpcId;
  RPCCallID callId;
  bool responseExpected;

  void Serialize(ByteWriter& ar) const
  {
    ar(rpcId);
    ar(callId);
    ar(responseExpected);
  }
  void Deserialize(ByteReader& ar)
  {
    ar(rpcId);
    ar(callId);
    ar(responseExpected);
  }
};
struct RPCResponseContext
{
  RPCID rpcId;
  RPCCallID callId;
  void Serialize(ByteWriter& ar) const
  {
    ar(rpcId);
    ar(callId);
  }
  void Deserialize(ByteReader& ar)
  {
    ar(rpcId);
    ar(callId);
  }
};
ATLASNET_MESSAGE(RPCRequestMessage,
                 ATLASNET_MESSAGE_DATA(RPCRequestContext, context),
                 ATLASNET_MESSAGE_DATA(std::vector<uint8_t>, payload));
ATLASNET_MESSAGE(RPCResponseMessage,
                 ATLASNET_MESSAGE_DATA(RPCResponseContext, context),
                 ATLASNET_MESSAGE_DATA(RPCResultW, result));

constexpr RPCID HashRPCName(const char* str)
{
  RPCID hash = 2166136261u;
  while (*str)
  {
    hash ^= static_cast<uint8_t>(*str++);
    hash *= 16777619u;
  }
  return hash;
}
template <std::size_t N> struct RPCFuncName
{
  char value[N];

  constexpr RPCFuncName(const char (&str)[N])
  {
    std::copy_n(str, N, value);
  }
};
} // namespace RPC_Internal
template <typename Archive, typename... Ts>
concept DefaultRPCSerializable =
    requires(Archive& ar, Ts&&... args) { ar(std::forward<Ts>(args)...); };
template <RPC_Internal::RPCFuncName Name, typename Return, typename... Args>
struct RPC
{
  using ReturnType = Return;
  using ArgsTuple = std::tuple<Args...>;
  static constexpr RPCID Id = RPC_Internal::HashRPCName(Name.value);
  static constexpr auto NameString = Name;

  template <typename Archive> static void serialize(Archive& ar, Args&&... args)
  {
    static_assert(
        DefaultRPCSerializable<Archive, Args...>,
        "The arguments of this RPC cannot be serialized using the default "
        "RPC serializer. Define a custom RPC::serialize() for this RPC.");
    ar(std::forward<Args>(args)...);
  }

  template <typename Func>
  static constexpr bool Invocable =
      std::is_void_v<Return> ? std::is_invocable_r_v<void, Func, Args...>
                             : std::is_invocable_r_v<ReturnType, Func, Args...>;

  template <typename Func>
  static constexpr bool ContextInvocable =
      std::is_void_v<Return>
          ? std::is_invocable_r_v<void, Func, const RPCContext&, Args...>
          : std::is_invocable_r_v<ReturnType, Func, const RPCContext&, Args...>;
};
/*if the function takes the args and returns the Return then its valid*/

using TestRPC = RPC<"TestRPC", void, int, float>;
} // namespace AtlasNet

/* #define ATLASNET_RPC_NEW(NameSpace, ServiceName, Return, ...) \
  struct ATLASNET_CAT3(NameSpace, _, ServiceName)                              \
  {                                                                            \
    using ReturnType = Return;                                                 \
    using ArgsTuple = std::tuple<__VA_ARGS__>;                                 \
    static constexpr std::string_view Name = #NameSpace "." #ServiceName;      \
    static constexpr RPCID Id =                                                \
        AtlasNet::RPC_Internal::HashRPCName(Name.data());                      \
    template <typename Archive> void serialize(Archive& ar) const {}           \
  };
 */