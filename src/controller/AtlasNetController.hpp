
#pragma once
#include "adapter/IServiceAdapter.hpp"
#include "atlasnet/controller/ControllerRPC.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/Singleton.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/heuristic/IHeuristic.hpp"
#include "atlasnet/core/job/JobContext.hpp"
#include "atlasnet/core/job/JobEnums.hpp"
#include "atlasnet/core/job/JobHandle.hpp"
#include "atlasnet/core/job/JobOptions.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/universe/UniverseEvents.hpp"
#include "atlasnet/core/universe/WorldConcepts.hpp"
#include "atlasnet/core/utils/DockerUtils.hpp"

#include "boost/multi_index/member.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/tag.hpp"
#include "boost/multi_index_container.hpp"
#include "enviroment/Enviroment.hpp"
#include "yaml-cpp/yaml.h"
#include <chrono>
#include <memory>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
namespace AtlasNet
{

class AtlasNetController : public IService
{



public:
  AtlasNetController();
  void OnInit() override;


private:
  void OnShutdown() override;

  void OnWorldCreated(const WorldCreatedEvent& event);

  void LoadStartupWorlds();

  std::unique_ptr<IServiceAdapter>
  CreateServiceAdapterForWorld(const WorldID& worldId);
  std::unordered_map<WorldID,std::unique_ptr<IServiceAdapter>> _serviceAdapters;
};
} // namespace AtlasNet
