#pragma once

#include "atlasnet/core/network/NetworkCommons.hpp"
#include "atlasnet/core/network/topology/ITopologyExecutor.hpp"
#include <future>
#include <string>
namespace AtlasNet::Network::Topology
{

class ITopologyAction
{
  NetworkNodeID nodeA;
  NetworkNodeID nodeB;
  ITopologyExecutor& executor;

public:
  ITopologyAction(ITopologyExecutor& executor, NetworkNodeID a, NetworkNodeID b)
      : executor(executor), nodeA(a), nodeB(b)
  {
  }
  virtual void Start() = 0;

  virtual bool Finished() = 0;

  virtual TopologyActionResult Result() = 0;

  NetworkNodeID GetNodeA() const
  {
    return nodeA;
  }
  NetworkNodeID GetNodeB() const
  {
    return nodeB;
  }
  ITopologyExecutor& GetExecutor() const
  {
    return executor;
  }
};

class ConnectAction : public ITopologyAction
{

public:
  ConnectAction(ITopologyExecutor& executor, NetworkNodeID a, NetworkNodeID b)
      : ITopologyAction(executor, a, b)
  {
  }
  std::atomic_bool success = false;
  std::atomic_bool finished = false;
  void Start() override
  {
    this->success = GetExecutor().SendConnectCommand(GetNodeA(), GetNodeB());
    this->finished = true;
  }

  bool Finished() override
  {
    return finished;
  }

  TopologyActionResult Result() override
  {
    TopologyActionResult result;
    result.success = success;
    return result;
  }
};
class WaitConnectAction : public ITopologyAction
{

public:
  WaitConnectAction(ITopologyExecutor& executor, NetworkNodeID a,
                    NetworkNodeID b)
      : ITopologyAction(executor, a, b)
  {
  }

  void Start() override {}

  bool Finished() override
  {
    return GetExecutor().GetConnectionState(GetNodeA(), GetNodeB()) ==
           SocketConnectionState::eConnected;
  }

  TopologyActionResult Result() override {}
};
class DisconnectAction : public ITopologyAction
{

public:
  DisconnectAction(ITopologyExecutor& executor, NetworkNodeID a,
                   NetworkNodeID b)
      : ITopologyAction(executor, a, b)
  {
  }
  void Start() override {}

  bool Finished() override {}

  TopologyActionResult Result() override {}
};
class WaitDisconnectAction : public ITopologyAction
{

public:
  WaitDisconnectAction(ITopologyExecutor& executor, NetworkNodeID a,
                       NetworkNodeID b)
      : ITopologyAction(executor, a, b)
  {
  }
  void Start() override {}

  bool Finished() override {}

  TopologyActionResult Result() override {}
};
} // namespace AtlasNet::Network::Topology