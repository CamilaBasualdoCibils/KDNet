#pragma once

#include "atlasnet/core/RPC/RPCMessage.hpp"

#include "RPCConcepts.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/tasks/TaskSystem.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "enviroment/Enviroment.hpp"
#include <functional>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <stack>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace AtlasNet
{

using RPCTarget = SocketAddress;
class RPCSystem
{
public:
  struct Config
  {
    std::optional<PortType>
        port; // if not specified then listens for requests on any port
    MessageSystem* messageSystem = nullptr;
  };

  RPCSystem(const Config& config);
  ~RPCSystem()
  {
    /* {
    logger->info("RPC destructor called, shutting down RPC and waiting for
    active jobs to complete...");

      std::unique_lock u(_activeJobsMutex);
      while (!activeJobs.empty())
      {
        if (!activeJobs.top().is_completed())
        {
        logger->info("Waiting for active job {} to complete... Current State:
    {}", activeJobs.top().name().value_or("<unnamed>"),
                     boost::describe::enum_to_string(activeJobs.top().state(),
                                                    "UNKNOWN"));

          activeJobs.top().wait();
        }

        activeJobs.pop();
      }
        logger->info("RPC destructor done, all active jobs have completed.");
    } */
  }

  template <typename MethodType, typename Func>
    requires RPC_Internal::BindableRpcHandler<MethodType, Func>
  void Bind(Func&& func);

  template <typename MethodType, typename... Args>
    requires(!std::is_void_v<typename MethodType::ReturnType>)
  [[nodiscard]] std::future<typename MethodType::ReturnType>
  Call(const RPCTarget& target, Args&&... args);

  template <typename MethodType, typename... Args>
    requires(std::is_void_v<typename MethodType::ReturnType>)
  void Call(const RPCTarget& target, Args&&... args);

private:
  void Shutdown();

  template <typename MethodType, typename... Args>
  std::pair<RPC_Internal::MethodID, RPC_Internal::CallID>
  SendRequest(const RPCTarget& target, Args&&... args);

  template <typename MethodType>
  void SendResponse(const RPCTarget& target, RPC_Internal::CallID callID,
                    const typename MethodType::ReturnType& ret);

  void SendError(const RPCTarget& target, RPC_Internal::MethodID methodId,
                 RPC_Internal::CallID callID, std::string errorMsg);

  void OnRPCRequest(const RpcRequestMessage& msg, const SocketAddress& address);
  void OnRPCResponse(const RpcResponseMessage& msg,
                     const SocketAddress& address);
  void OnRPCError(const RpcErrorMessage& msg, const SocketAddress& address);

  RPC_Internal::CallID GetNextCallID(RPC_Internal::MethodID methodId)
  {
    auto& next = _nextCallID[methodId];
    return next++;
  }
  /* void NewActiveJob(JobHandle handle)
  {
    std::unique_lock u(_activeJobsMutex);
    for (int i = 0; i < activeJobs.size(); ++i)
    {
      if (activeJobs.top().is_completed())
      {
        activeJobs.pop();
      }
    }
    activeJobs.push(handle);
  } */
  struct PendingPromiseKey
  {
    RPC_Internal::MethodID methodId;
    RPC_Internal::CallID callId;

    bool operator==(const PendingPromiseKey& other) const noexcept
    {
      return methodId == other.methodId && callId == other.callId;
    }
  };

  struct PendingPromiseKeyHash
  {
    std::size_t operator()(const PendingPromiseKey& k) const noexcept
    {
      std::size_t h1 = std::hash<RPC_Internal::MethodID>{}(k.methodId);
      std::size_t h2 = std::hash<RPC_Internal::CallID>{}(k.callId);
      return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
  };

  using BindFunc = std::function<void(const RPCTarget&, RPC_Internal::CallID,
                                      std::span<const uint8_t>)>;

  struct PendingRequest
  {
    std::function<void(std::span<const uint8_t>)> onResponse;
    std::function<void(const std::string&)> onError;
  };

  std::unordered_map<RPC_Internal::MethodID, BindFunc> _methodBindHandlers;
  std::unordered_map<RPC_Internal::MethodID, RPC_Internal::CallID> _nextCallID;
  std::unordered_map<PendingPromiseKey, PendingRequest, PendingPromiseKeyHash>
      _pendingPromises;
  // std::shared_mutex _activeJobsMutex;
  // std::stack<JobHandle> activeJobs;

  std::shared_mutex _mutex;
  const Config config_;
  std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("RPC");
};

} // namespace AtlasNet

template <typename MethodType, typename... Args>
inline std::pair<AtlasNet::RPC_Internal::MethodID,
                 AtlasNet::RPC_Internal::CallID>
AtlasNet::RPCSystem::SendRequest(const RPCTarget& target, Args&&... args)
{
  ByteWriter writeArgs;
  writeArgs(std::forward<Args>(args)...);

  RpcRequestMessage request{.methodId = MethodType::Id,
                            .callID = GetNextCallID(MethodType::Id),
                            .payload =
                                std::vector<uint8_t>(writeArgs.bytes().begin(),
                                                     writeArgs.bytes().end())};

  auto sendRequestHandle = config_.messageSystem->QueueMessage(
      request, target, MessageSendMode::eReliableBatched);
  // NewActiveJob(sendRequestHandle);
  return std::make_pair(request.methodId, request.callID);
}

template <typename MethodType>
inline void
AtlasNet::RPCSystem::SendResponse(const RPCTarget& target,
                                  RPC_Internal::CallID callID,
                                  const typename MethodType::ReturnType& ret)
{
  ByteWriter writeArgs;
  writeArgs(ret);

  RpcResponseMessage response{
      .methodId = MethodType::Id,
      .callID = callID,
      .payload = std::vector<uint8_t>(writeArgs.bytes().begin(),
                                      writeArgs.bytes().end())};

  auto sendResponseHandle = config_.messageSystem->QueueMessage(
      response, target, MessageSendMode::eReliableBatched);
  sendResponseHandle->wait();
  assert(sendResponseHandle.GetTask().is_done() && 
         "Failed to send RPC request message and no fault scenarios have been "
         "implemented");
  logger->info("Sent RPC response for methodId {} callId {} to {}",
               response.methodId, response.callID, target.to_string());
  // NewActiveJob(sendResponseHandle);
}

template <typename MethodType, typename... Args>
  requires(std::is_void_v<typename MethodType::ReturnType>)
inline void AtlasNet::RPCSystem::Call(const RPCTarget& target, Args&&... args)
{
  SendRequest<MethodType>(target, std::forward<Args>(args)...);
}

template <typename MethodType, typename... Args>
  requires(!std::is_void_v<typename MethodType::ReturnType>)
std::future<typename MethodType::ReturnType>
AtlasNet::RPCSystem::Call(const RPCTarget& target, Args&&... args)
{
  using ReturnType = typename MethodType::ReturnType;

  auto promise = std::make_shared<std::promise<ReturnType>>();
  auto future = promise->get_future();

  RPC_Internal::MethodID methodId = MethodType::Id;
  RPC_Internal::CallID callID;

  {
    // first check if lock is taken in this scope, if it is then warn
    if (_mutex.try_lock())
    {
      _mutex.unlock();
    }
    else
    {
      logger->warn("RPCSystem::Call is waiting for lock. This may indicate a "
                   "deadlock or long-running RPC handler.");
    }
    std::unique_lock lock(_mutex);
    callID = GetNextCallID(methodId);

    PendingRequest pending;
    pending.onResponse = [promise](std::span<const uint8_t> payload) mutable
    {
      auto serializeFunction =
          [](std::span<const uint8_t> payload) -> ReturnType
      {
        ByteReader reader(payload);
        ReturnType value{};
        reader(value);
        return value;
      };
      if (Env::DebugMode)
      {
        try
        {
          ByteReader reader(payload);
          ReturnType value{};
          reader(value);
          promise->set_value(std::move(value));
        }
        catch (...)
        {
          promise->set_exception(std::current_exception());
        }
      }
      else
      {
        serializeFunction(payload);
      }
    };

    pending.onError = [promise](const std::string& errorMsg) mutable
    {
      promise->set_exception(
          std::make_exception_ptr(std::runtime_error(errorMsg)));
    };

    _pendingPromises.emplace(PendingPromiseKey{methodId, callID},
                             std::move(pending));
  }

  ByteWriter writeArgs;
  writeArgs(std::forward<Args>(args)...);

  RpcRequestMessage request{.methodId = methodId,
                            .callID = callID,
                            .payload =
                                std::vector<uint8_t>(writeArgs.bytes().begin(),
                                                     writeArgs.bytes().end())};
  logger->info("Sending RPC request for methodId {} callId {} to {}", methodId,
               callID, target.to_string());
  auto sendMessageJobHandle = config_.messageSystem->QueueMessage(
      request, target, MessageSendMode::eReliableBatched);
  /* sendMessageJobHandle.wait();
  assert(sendMessageJobHandle.is_completed() &&
         "Failed to send RPC request message and no fault scenarios have been "
         "implemented"); */

  return future;
}

template <typename MethodType, typename Func>
  requires AtlasNet::RPC_Internal::BindableRpcHandler<MethodType, Func>
inline void AtlasNet::RPCSystem::Bind(Func&& func)
{
  using ReturnType = std::remove_cvref_t<typename MethodType::ReturnType>;
  using ArgsTuple = typename MethodType::ArgsTuple;

  std::unique_lock lock(_mutex);
  _methodBindHandlers[MethodType::Id] =
      [f = std::forward<Func>(func),
       this](const RPCTarget& caller, RPC_Internal::CallID callId,
             std::span<const uint8_t> payload) mutable
  {
    /*  try
     { */
    ByteReader reader(payload);

    ArgsTuple args;
    std::apply([&](auto&... arg) { (reader(arg), ...); }, args);

    if constexpr (std::is_void_v<ReturnType>)
    {
      std::apply(f, args);
    }
    else
    {
      ReturnType result = std::apply(f, args);
      SendResponse<MethodType>(caller, callId, result);
    }
    /* }
    catch (const std::exception& e)
    {
      SendError(caller, MethodType::Id, callId, e.what());
    }
    catch (...)
    {
      SendError(caller, MethodType::Id, callId, "Unhandled RPC exception");
    } */
  };
  logger->info("Bound RPC method {} with MethodID {}", MethodType::GetName(),
               MethodType::Id);
}
