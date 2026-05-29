#pragma once
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/database/redis/RedisConn.hpp"
#include "atlasnet/core/events/GlobalEventSystem.hpp"
#include "atlasnet/core/events/LocalEventSystem.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/service/ServiceRegistry.hpp"
#include "atlasnet/core/universe/Universe.hpp"
#include <atomic>
#include <boost/describe.hpp>
#include <cassert>
#include <condition_variable>

#include <mutex>
#include <unistd.h>
namespace AtlasNet
{

class IService
{
protected:
  IService(ServiceType type);
  virtual ~IService() = default;
  bool ShutdownRequested() const;
  virtual void OnInit() = 0;
  virtual void OnUpdate() = 0;
  virtual void OnShutdown() = 0;

  // HostAddress GetOverlayAddressOfSelf() const;
  HostName GetHostName() const
  {
    return HostName(std::string(std::getenv("HOSTNAME")));
  }
  const ServiceID& GetContainerID() const
  {
    return id;
  }
  RPCSystem& GetRPCSystem()
  {
    assert(_rpcSystem.has_value() && "RPCSystem not initialized");
    return _rpcSystem.value();
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
  JobSystem& GetJobSystem()
  {
    assert(_jobSystem.has_value() && "JobSystem not initialized");
    return _jobSystem.value();
  }
  ServiceRegistry& GetServiceRegistry()
  {
    assert(_serviceRegistry.has_value() && "ServiceRegistry not initialized");
    return _serviceRegistry.value();
  }
  Universe& GetUniverse()
  {
    assert(_universe.has_value() && "Universe not initialized");
    return _universe.value();
  }

public:
  void Init();
  ServiceType GetServiceType() const
  {
    return type;
  }
  ServiceID GetID() const
  {
    return id;
  }
  HostAddress GetControllerAddress() const
  {
    assert(controllerOverlayAddress.has_value() &&
           "Controller address not set. This should never happen as "
           "non-controller "
           "containers fetch the controller info during initialization.");
    return controllerOverlayAddress.value();
  }
  ServiceID GetControllerID() const
  {
    assert(controllerContainerID.has_value() &&
           "Controller container ID not set. This should never happen as "
           "non-controller "
           "containers fetch the controller info during initialization.");
    return controllerContainerID.value();
  }

private:
  void FetchControllerInfo();
  ServiceID id{ServiceID::Generate()};
  ServiceType type;
  std::atomic<bool> shutdown{false};
  std::mutex mutex;
  std::condition_variable cv;

  std::optional<JobSystem> _jobSystem;
  std::optional<LocalEventSystem> _eventSystem;
  std::optional<GlobalEventSystem> _globalEventSystem;
  std::optional<MessageSystem> _messageSystem;
  std::optional<MessageSystem::ListenSocketHandle*> _internalMessageSocket;
  std::optional<RPCSystem> _rpcSystem;
  std::optional<Universe> _universe;
  std::optional<ServiceRegistry> _serviceRegistry;
  std::unique_ptr<Database::RedisConn> _redisDatabase;

  std::optional<ServiceID> controllerContainerID;
  std::optional<HostAddress> controllerOverlayAddress;
  // Database::InternalDB _internalDB;
  static inline IService& Get()
  {
    static IService& instance = []() -> IService&
    {
      throw std::runtime_error(
          "IContainer instance not set. Create a concrete container class that "
          "inherits from IContainer and set it up before calling Get().");
    }();
    return instance;
  }
};

} // namespace AtlasNet