#pragma once

#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/messages/Message.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/utils/MacroConcepts.hpp"
#include <cstdint>
#include <expected>
#include <span>

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
  None,

  Timeout,
  UnknownRPC,
  InvalidPayload,
  PermissionDenied,
  RemoteException,
};
using RPCResult = std::expected<std::vector<uint8_t>, RPCError>;

using RPC_BindCallFunction_Raw =
    std::function<RPCResult(const RPCContext&, std::span<const uint8_t>)>;

namespace RPC_Internal
{

struct RPCInternalContext
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
/* ATLASNET_MESSAGE(RPCRequestMessage,
                 ATLASNET_MESSAGE_DATA(RPCInternalContext, context),
                 ATLASNET_MESSAGE_DATA(std::vector<uint8_t>, payload));
ATLASNET_MESSAGE(RPCResponseMessage,
                 ATLASNET_MESSAGE_DATA(RPCInternalContext, context),
                 ATLASNET_MESSAGE_DATA(RPCResult, result)); */

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

} // namespace RPC_Internal
} // namespace AtlasNet

#define ATLASNET_RPC_NEW(NameSpace, ServiceName, Return, ...)                  \
  struct ATLASNET_CAT3(NameSpace, _, ServiceName)                              \
  {                                                                            \
    using ReturnType = Return;                                                 \
    using ArgsTuple = std::tuple<__VA_ARGS__>;                                 \
    static constexpr std::string_view Name = #NameSpace "." #ServiceName;      \
    static constexpr RPCID Id =                                                \
        AtlasNet::RPC_Internal::HashRPCName(Name.data());                      \
  };
