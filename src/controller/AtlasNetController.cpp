
#include "AtlasNetController.hpp"
#include "adapter/IServiceAdapter.hpp"
#include "adapter/KubernetesServiceAdapter.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/universe/WorldConcepts.hpp"
#include "atlasnet/core/universe/WorldEnums.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "boost/describe/enumerators.hpp"
#include "enviroment/Enviroment.hpp"
#include <format>
#include <utility>

void AtlasNet::AtlasNetController::OnInit()
{
  std::cerr << "Initializing AtlasNet Controller..." << std::endl;

  GetServiceRegistry().RegisterService(ServiceRegistry::ServiceInfo{
      .id = GetContainerID(),
      .address = GetHostName(),
      .containerType = ServiceType::Controller,
  });
  GetGlobalEventSystem().On<WorldCreatedEvent>(
      [this](const WorldCreatedEvent& event) { OnWorldCreated(event); });
  LoadStartupWorlds();
}
void AtlasNet::AtlasNetController::OnShutdown()
{
  std::cerr << "Shutting down AtlasNet Controller..." << std::endl;
  for (auto& [worldID, adapter] : _serviceAdapters)
  {
    std::cerr << "Destroying service adapter for world ID "
              << worldID.to_string() << std::endl;
    adapter->Destroy();
    std::cerr << "Destroyed service adapter for world ID "
              << worldID.to_string() << std::endl;
  }
}
void AtlasNet::AtlasNetController::OnUpdate() {}
AtlasNet::AtlasNetController::AtlasNetController()
    : IService(ServiceType::Controller)
{
}
void AtlasNet::AtlasNetController::OnWorldCreated(
    const WorldCreatedEvent& event)
{
  std::cerr << "Received WorldCreatedEvent for world " << event.worldName
            << " with ID " << event.worldID.to_string() << std::endl;
  _serviceAdapters.emplace(event.worldID,
                           CreateServiceAdapterForWorld(event.worldID));
  std::cerr << "Created service adapter for world " << event.worldName
            << " with ID " << event.worldID.to_string() << std::endl;
  _serviceAdapters[event.worldID]->Create();
  std::cerr << "Called Create on service adapter for world " << event.worldName
            << " with ID " << event.worldID.to_string() << std::endl;
}

void AtlasNet::AtlasNetController::LoadStartupWorlds()
{
  /*
 example of expected format for Env::StartupWorlds:
{
 "worlds": {
   "MainWorld": {
     "SpaceType": "Cartesian2D",
     "Heuristic": "KDTree"
   },
   "AnotherWorld": {
     "SpaceType": "Cartesian3D",
     "ShardImageOverride": "custom-shard-image:latest"
   }
 }
}
 */
  try
  {

    _JsonOrdered startupWorlds = _JsonOrdered::parse(Env::StartupWorlds);

    for (auto& [worldName, worldConfig] : startupWorlds["worlds"].items())
    {
      std::cerr << "Loading startup world: " << worldName << std::endl;
      WorldDefinition worldDef;
      worldDef.name = worldName;
      if (worldConfig.contains("SpaceType"))
      {
        std::string spaceTypeStr = worldConfig["SpaceType"].get<std::string>();
        if (spaceTypeStr == "Cartesian2D")
          worldDef.spaceType = WorldSpaceType::Cartesian2D;
        else if (spaceTypeStr == "Cartesian3D")
          worldDef.spaceType = WorldSpaceType::Cartesian3D;
        else
          std::cerr << "Unknown SpaceType '" << spaceTypeStr << "' for world "
                    << worldName << ". Defaulting to Cartesian2D." << std::endl;
      }
      else
      {
        std::cerr << "No SpaceType specified for world " << worldName
                  << ". Defaulting to Cartesian2D." << std::endl;
        worldDef.spaceType = WorldSpaceType::Cartesian2D;
      }
      auto [result, worldID] = GetUniverse().CreateWorld(worldDef);
      std::cerr << std::format(
                       "CreateWorld result: {}\n - worldID: {}\n - worldName: "
                       "{}\n, - SpaceType: {}",
                       boost::describe::enum_to_string(result,
                                                       "UNKNOWN RESULT"),
                       worldID->to_string(), worldName,
                       boost::describe::enum_to_string(worldDef.spaceType,
                                                       "UNKNOWN SPACE TYPE"))
                << std::endl;
    }
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Failed to load startup worlds: " << ex.what() << std::endl;
    std::cerr << "Startup worlds config: " << Env::StartupWorlds << std::endl;
  }
}
std::unique_ptr<AtlasNet::IServiceAdapter>
AtlasNet::AtlasNetController::CreateServiceAdapterForWorld(
    const WorldID& worldId)
{
  std::string ServiceName = "atlasnet-shard-" + worldId.to_string();
  // try to detect if this is running in kubernetes
  if (std::getenv("KUBERNETES_SERVICE_HOST"))
  {
    std::cerr << "Detected Kubernetes environment, using "
                 "KubernetesServiceAdapter for world "
              << ServiceName << std::endl;
    return std::make_unique<KubernetesServiceAdapter>(
        ServiceName, Env::ShardDefaultImage,
        KubernetesServiceAdapter::ImagePullPolicy::Never);
  }
  else
  {
    std::cerr << "No Kubernetes environment detected, using default "
                 "IServiceAdapter for world "
              << ServiceName << std::endl;
    throw std::runtime_error("No suitable service adapter found for world " +
                             ServiceName);
  }
}
