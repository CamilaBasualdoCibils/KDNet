#pragma once

#include "atlasnet/core/network/topology/ITopologyAction.hpp"
#include "taskflow/core/executor.hpp"
#include "taskflow/core/task.hpp"
#include "taskflow/core/taskflow.hpp"
namespace AtlasNet::Network::Topology
{
class TopologyTransition
{
public:
  using ActionID = size_t;

private:
  struct ActionNode
  {
    std::unique_ptr<ITopologyAction> action;
    tf::Task task;
    bool completed = false;
  };
  tf::Taskflow taskflow;
  ITopologyExecutor& executor;

  std::vector<ActionNode> actions;

public:
  TopologyTransition(ITopologyExecutor& executor) : executor(executor) {}
  template <typename T, typename... Args> ActionID Add(Args&&... args)
  {
    static_assert(std::is_base_of_v<ITopologyAction, T>);

    ActionID id = actions.size();

    auto task = taskflow.emplace([this, id]() { actions[id].action->Start(); });

    actions.push_back(
        {std::make_unique<T>(executor, std::forward<Args>(args)...), task});

    return id;
  }
  ActionID AddConnectAction(NetworkNodeID a, NetworkNodeID b)
  {
    return Add<ConnectAction>(a, b);
  }
  ActionID AddWaitConnectAction(NetworkNodeID a, NetworkNodeID b)
  {
    return Add<WaitConnectAction>(a, b);
  }
  ActionID AddDisconnectAction(NetworkNodeID a, NetworkNodeID b)
  {
    return Add<DisconnectAction>(a, b);
  }
  ActionID AddWaitDisconnectAction(NetworkNodeID a, NetworkNodeID b)
  {
    return Add<WaitDisconnectAction>(a, b);
  }
  void DependsOn(ActionID before, ActionID after)
  {
    actions[before].task.precede(actions[after].task);
  }
  tf::Taskflow& GetTaskflow()
  {
    return taskflow;
  }
  void Tick()
  {
    for (auto& action : actions)
    {
      if (action.action->Finished())
      {
        action.completed = true;
      }
    }
  }
  void Execute()
  {
    tf::Executor executor;
    executor.run(taskflow).wait();
  }
};
}; // namespace AtlasNet::Network::Topology