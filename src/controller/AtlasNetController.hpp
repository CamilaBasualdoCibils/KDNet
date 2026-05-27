
#pragma once
#include "atlasnet/agent/AgentRPC.hpp"
#include "atlasnet/controller/ControllerRPC.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/Singleton.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/universe/UniverseEvents.hpp"
#include "atlasnet/core/utils/DockerUtils.hpp"

#include "boost/multi_index/member.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/tag.hpp"
#include "boost/multi_index_container.hpp"
#include "yaml-cpp/yaml.h"
#include <queue>
#include <stdexcept>
namespace AtlasNet
{

class AtlasNetController : public IContainer
{

  struct AgentEntry
  {
    ContainerID id;
    HostAddress address;
    uint32_t cpuTotalCount;
    uint32_t cpuAvailCount;
  };

public:
  AtlasNetController() : IContainer(ContainerType::Controller) {}
  void OnInit() override;
  void OnUpdate() override {}

private:
  void OnShutdown() override
  {
    std::cerr << "Shutting down AtlasNet Controller..." << std::endl;
  }

  void OnAgentReady(const DeclareSelfAgentReadyRequest& request)
  {
    std::cerr << "Agent " << request.agentID.to_string() << " is ready at "
              << request.agentAddress.to_string() << " with "
              << request.cpuCount << " CPU cores." << std::endl;
    AgentEntry entry{.id = request.agentID,
                     .address = request.agentAddress,
                     .cpuTotalCount = request.cpuCount,
                     .cpuAvailCount = request.cpuCount};
    agentSet.insert(entry);
    // Add the entry to a list or map of agents if needed
  }
  void OnWorldCreated(const WorldCreatedEvent& event)
  {
    std::cerr << "Received WorldCreatedEvent for world " << event.worldName
              << " with ID " << event.worldID.to_string() << std::endl;
    SpawnShard();
  }
  void SpawnShard()
  {
    // Find the agent with the most available CPU cores
    auto& index = agentSet.get<AgentByCpuAvailTag>();
    if (index.empty())
    {
      std::cerr << "No agents available to spawn shard." << std::endl;
      return;
    }
    const auto& bestAgent = index.begin();
    std::cerr << "Spawning shard on agent " << bestAgent->id.to_string()
              << " at " << bestAgent->address.to_string() << std::endl;

    agentSet.modify(
        index.begin(),
        [](AgentEntry& entry)
        {
          entry.cpuAvailCount -=
              Env::ShardCPUReserve; // Reserve CPU cores for the new shard
        });
    // Here you would add the logic to actually spawn the shard on the selected
    // agent, such as sending an RPC command to the agent to start a new shard
    // process/container.
  }

  struct AgentByCpuAvailTag
  {
  };
  struct AgentByCpuTotalTag
  {
  };
  boost::multi_index::multi_index_container<
      AgentEntry, boost::multi_index::indexed_by<
                      boost::multi_index::ordered_non_unique<
                          boost::multi_index::tag<AgentByCpuAvailTag>,
                          boost::multi_index::member<
                              AgentEntry, uint32_t, &AgentEntry::cpuAvailCount>,
                          std::greater<uint32_t>>,
                      boost::multi_index::ordered_non_unique<
                          boost::multi_index::tag<AgentByCpuTotalTag>,
                          boost::multi_index::member<
                              AgentEntry, uint32_t, &AgentEntry::cpuTotalCount>,
                          std::greater<uint32_t>>>>
      agentSet;
};
} // namespace AtlasNet
