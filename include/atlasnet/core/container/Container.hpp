#pragma once
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/Singleton.hpp"
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/UUID.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "database/internal/InternalDB.hpp"
#include <atomic>
#include <boost/describe.hpp>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
namespace AtlasNet
{

struct ContainerIDTag
{
};
using ContainerID = StrongUUID<ContainerIDTag>;
enum class ContainerType
{
  Controller,
  Agent,
  Shard,
  Proxy
};
BOOST_DESCRIBE_ENUM(ContainerType, Controller, Agent, Shard, Proxy);
class IContainer
{

protected:
  IContainer(ContainerType type);
  bool ShutdownRequested() const;
  virtual void OnInit() = 0;
  virtual void OnShutdown()  = 0;

  HostAddress GetOverlayAddressOfSelf() const;

  RPCSystem& GetRPCSystem()
  {
    return _rpcSystem;
  }
  MessageSystem& GetMessageSystem()
  {
    return _messageSystem;
  }
  JobSystem& GetJobSystem()
  {
    return _jobSystem;
  }
public:
  void Init();

private:
  ContainerID id{ContainerID::Generate()};
  ContainerType type;
  std::atomic<bool> shutdown{false};
  std::mutex mutex;
  std::condition_variable cv;

  JobSystem _jobSystem{JobSystem::Config{}};
  MessageSystem _messageSystem{MessageSystem::Config{.jobSystem = &_jobSystem}};
  RPCSystem _rpcSystem{RPCSystem::Config{.messageSystem = &_messageSystem}};

  Database::InternalDB _internalDB;
  static inline IContainer& Get()
  {
    static IContainer& instance = []() -> IContainer&
    {
      throw std::runtime_error(
          "IContainer instance not set. Create a concrete container class that "
          "inherits from IContainer and set it up before calling Get().");
    }();
    return instance;
  }
};

} // namespace AtlasNet