#pragma once

#include "atlasnet/core/RPC/new/RPCConcepts_N.hpp"
#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include <future>
#include <shared_mutex>
#include <unordered_map>
namespace AtlasNet
{

class RPC
{

public:
  struct Config
  {
    MessageSystem* messageSystem = nullptr;
    std::chrono::milliseconds timeout = std::chrono::seconds(5);
  };
  RPC(const Config& config) : config_(config)
  {
    assert(config_.messageSystem &&
           "RPCSystem requires a MessageSystem in its config");
    config_.messageSystem->On<RPCRequestMessage>(
        [this](const RPCRequestMessage& request,
               const SocketAddress& sender) {});
  }
  void Bind(std::string_view name, RPC_BindCallFunction_Raw func)
  {
    const RPCID id = RPC_Internal::HashRPCName(name.data());
    Bind(id, std::move(func));
  }
  void Bind(RPCID id, RPC_BindCallFunction_Raw func)
  {
    std::unique_lock lock(mutex);
    bindings[id] = std::move(func);
  }

  void Call(SocketAddress target, std::string_view methodName,
            std::span<const uint8_t> payload)
  {
    const RPCID methodId = RPC_Internal::HashRPCName(methodName.data());
    Call(target, methodId, payload);
  }
  void Call(SocketAddress target, RPCID methodId,
            std::span<const uint8_t> payload)
  {
    RPCCallID callId = nextCallID++;
    _SendCallRequest(target,
                     RPC_Internal::RPCInternalContext{
                         .callId = callId,
                         .responseExpected = false,
                     },
                     payload);
  }

  std::future<RPCResult> Call_R(SocketAddress target,
                                std::string_view methodName,
                                std::span<const uint8_t> payload)
  {
    const RPCID methodId = RPC_Internal::HashRPCName(methodName.data());
    return Call_R(target, methodId, payload);
  }
  std::future<RPCResult> Call_R(SocketAddress target, RPCID methodId,
                                std::span<const uint8_t> payload)
  {
    RPCCallID callId = nextCallID++;
    std::unique_lock<std::shared_mutex> lock(mutex);
    std::future<RPCResult> future = pendingCalls[callId].get_future();
    mutex.unlock();
    _SendCallRequest(target,
                     RPC_Internal::RPCInternalContext{
                         .callId = callId,
                         .responseExpected = true,
                     },
                     payload);
    return future;
  }

private:
  void _SendCallRequest(SocketAddress target,
                        RPC_Internal::RPCInternalContext int_context,
                        std::span<const uint8_t> payload)
  {
  }
  const Config config_;
  std::shared_mutex mutex;
  std::unordered_map<RPCID, RPC_BindCallFunction_Raw> bindings;
  std::atomic<RPCCallID> nextCallID{0};
  std::unordered_map<RPCCallID, std::promise<RPCResult>> pendingCalls;
};
}; // namespace AtlasNet