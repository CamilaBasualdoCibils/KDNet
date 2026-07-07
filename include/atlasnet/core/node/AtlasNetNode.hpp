#pragma once
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/Snowflake.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
#include "atlasnet/core/database/redis/RedisConn.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/events/GlobalEventSystem.hpp"
#include "atlasnet/core/events/LocalEventSystem.hpp"
#include "atlasnet/core/tasks/TaskSystem.hpp"
#include "atlasnet/core/messages/HandshakePacket.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/node/NodeRegistry.hpp"
#include "atlasnet/core/universe/Universe.hpp"
#include <atomic>
#include <boost/describe.hpp>
#include <cassert>
#include <condition_variable>

#include <cstdlib>
#include <mutex>
#include <unistd.h>
namespace AtlasNet
{

class IAtlasNetNode
{
protected:
  IAtlasNetNode(AtlasNetNodeType type);
  virtual ~IAtlasNetNode() = default;
  bool ShutdownRequested() const;
  void Shutdown()
  {
    OnShutdown();

    _messageSystem->Shutdown();
    _taskSystem->Shutdown();
  }
  virtual void OnInit() = 0;
  virtual void OnShutdown() = 0;

  virtual HandshakeResponsePacket
  HandleHandshake(const HandshakeIdentity& identity,
                  const SocketAddress& remoteAddr)
  {
    return HandshakeResponsePacket{.accepted = true};
    /* if (identity.role == HandshakeRole::eServer)
    {
      HandshakeServerRequestData requestData =
          std::get<HandshakeServerRequestData>(identity.data);
      std::optional<NodeRegistry::ServiceInfo> serviceInfo =
          GetServiceRegistry().GetServiceInfo(requestData.serviceID);

      if (!serviceInfo)
      {
        logger->error("Handshake failed: requested service ID {} not found in registry",
                     requestData.serviceID.to_string());
        return HandshakeResponsePacket{.accepted = false,
                                       .rejectReason =
                                           "Requested service ID not found"};
      }
      return HandshakeResponsePacket{.accepted = true};
    }
    else
    {

      return HandshakeResponsePacket{
          .accepted = false,
          .rejectReason = "Client handshakes not supported in base IService"};
    } */
  }
  // HostAddress GetOverlayAddressOfSelf() const;
  SocketAddress GetNetworkAddress() const;
  RPCSystem& GetRPCSystem()
  {
    assert(_rpcSystem.has_value() && "RPCSystem not initialized");
    return _rpcSystem.value();
  }
  spdlog::logger* GetLogger() const
  {
    return logger.get();
  }
  LocalEventSystem& GetLocalEventSystem()
  {
    assert(_eventSystem.has_value() && "LocalEventSystem not initialized");
    return _eventSystem.value();
  }
  GlobalEventSystem& GetGlobalEventSystem()
  {
    assert(_globalEventSystem.has_value() &&
           "GlobalEventSystem not initialized");
    return _globalEventSystem.value();
  }
  MessageSystem& GetMessageSystem()
  {
    assert(_messageSystem.has_value() && "MessageSystem not initialized");
    return _messageSystem.value();
  }
  TaskSystem& GetTaskSystem()
  {
    assert(_taskSystem.has_value() && "TaskSystem not initialized");
    return _taskSystem.value();
  }
  NodeRegistry& GetServiceRegistry()
  {
    assert(_serviceRegistry.has_value() && "ServiceRegistry not initialized");
    return _serviceRegistry.value();
  }
  Universe& GetUniverse()
  {
    assert(_universe.has_value() && "Universe not initialized");
    return _universe.value();
  }
  Database::RedisConn& GetRedisConn()
  {
    assert(_redisDatabase && "RedisConn not initialized");
    return *_redisDatabase;
  }

public:
  void Init();
  AtlasNetNodeType GetNodeType() const
  {
    return type;
  }
  AtlasNetNodeID GetNodeID() const
  {
    assert(id.has_value() && "ID not set");
    return *id;
  }


private:

  std::optional<AtlasNetNodeID> id;
  const AtlasNetNodeType type;

std::shared_ptr<spdlog::logger> logger;
  std::atomic<bool> shutdown_requested{false};
  std::mutex mutex;
  std::condition_variable cv;

  std::optional<TaskSystem> _taskSystem;
  std::optional<LocalEventSystem> _eventSystem;
  std::optional<GlobalEventSystem> _globalEventSystem;
  std::optional<MessageSystem> _messageSystem;
  std::optional<MessageSystem::ListenSocketHandle*> _internalMessageSocket;
  std::optional<RPCSystem> _rpcSystem;
  std::optional<Universe> _universe;
  std::optional<NodeRegistry> _serviceRegistry;
  std::unique_ptr<Database::RedisConn> _redisDatabase;

  // Database::InternalDB _internalDB;
  static inline IAtlasNetNode& Get()
  {
    static IAtlasNetNode& instance = []() -> IAtlasNetNode&
    {
      throw std::runtime_error(
          "IContainer instance not set. Create a concrete container class that "
          "inherits from IContainer and set it up before calling Get().");
    }();
    return instance;
  }
};

} // namespace AtlasNet