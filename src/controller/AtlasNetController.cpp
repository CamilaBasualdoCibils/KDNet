
#include "AtlasNetController.hpp"
#include "adapter/DockerServiceAdapter.hpp"
#include "adapter/IServiceAdapter.hpp"
#include "adapter/KubernetesServiceAdapter.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/universe/WorldConcepts.hpp"
#include "atlasnet/core/universe/WorldEnums.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "enviroment/Enviroment.hpp"
#include <format>
#include <utility>

void AtlasNet::AtlasNetController::OnInit()
{
  std::cerr << "Initializing AtlasNet Controller..." << std::endl;

  if (Env::ControllerServiceBackend == ServiceAdapterType::INVALID)
  {
    std::cerr
        << "Warning: Invalid service adapter type specified in "
           "ATLASNET_CONTROLLER_SERVICE_BACKEND. No service adapters will "
           "be created."
        << std::endl;
    std::cerr << "Received value: "
              << std::getenv("ATLASNET_CONTROLLER_SERVICE_BACKEND")
              << std::endl;
    throw std::runtime_error(
        "Invalid service adapter type specified in environment variable "
        "ATLASNET_CONTROLLER_SERVICE_BACKEND");
  }
  GetServiceRegistry().RegisterService(ServiceRegistry::ServiceInfo{
      .id = GetContainerID(),
      .address = GetHostName(),
      .containerType = ServiceType::Controller,
  });
  GetGlobalEventSystem().On<WorldCreatedEvent>(
      [this](const WorldCreatedEvent& event) { OnWorldCreated(event); });

  std::cerr << "Creating proxy service adapter..." << std::endl;
  _ProxyServiceAdapter =
      CreateServiceAdapter("atlasnet-proxy", "atlasnet-proxy-dev",
                           {{Env::ProxyListenPort, Env::ProxyListenPort}});
  _ProxyServiceAdapter->Create();
  std::cerr << "Scaling proxy service adapter to 1 replica..." << std::endl;
  _ProxyServiceAdapter->SetReplicaCount(1);
  LoadStartupWorlds();
}
void AtlasNet::AtlasNetController::OnShutdown()
{
  std::unique_lock lock(_adapterMutex);
  std::cerr << "Shutting down AtlasNet Controller..." << std::endl;
  for (auto& [worldID, adapter] : _serviceAdapters)
  {
    std::cerr << "Destroying service adapter for world ID "
              << worldID.to_string() << std::endl;
    adapter->Destroy();
    std::cerr << "Destroyed service adapter for world ID "
              << worldID.to_string() << std::endl;
  }
  _serviceAdapters.clear();
  if (_ProxyServiceAdapter)
  {
    std::cerr << "Destroying proxy service adapter..." << std::endl;
    _ProxyServiceAdapter->Destroy();
    std::cerr << "Destroyed proxy service adapter." << std::endl;
  }
}
AtlasNet::AtlasNetController::AtlasNetController()
    : IService(ServiceType::Controller)
{
}
void AtlasNet::AtlasNetController::OnWorldCreated(
    const WorldCreatedEvent& event)
{
  IServiceAdapter* adapter = nullptr;
  {
    std::unique_lock lock(_adapterMutex);
    std::cerr << "Received WorldCreatedEvent for world " << event.worldName
              << " with ID " << event.worldID.to_string() << std::endl;
    _serviceAdapters.emplace(
        event.worldID,
        CreateServiceAdapter("atlasnet-shard-" + event.worldID.to_string(),
                             Env::ShardDefaultImage));
    adapter = _serviceAdapters[event.worldID].get();
  }
  std::cerr << "Created service adapter for world " << event.worldName
            << " with ID " << event.worldID.to_string() << std::endl;
  adapter->Create();
  std::cerr << "Called Create on service adapter for world " << event.worldName
            << " with ID " << event.worldID.to_string() << std::endl;
  adapter->SetReplicaCount(1);
  std::cerr << "Set replica count to 1 on service adapter for world "
            << event.worldName << " with ID " << event.worldID.to_string()
            << std::endl;
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
AtlasNet::AtlasNetController::CreateServiceAdapter(
    const std::string_view& serviceName, const std::string_view& imageName,
    std::vector<std::pair<PortType, PortType>> portMappings)
{
  std::string ServiceName = std::string(serviceName);
  // try to detect if this is running in kubernetes
  std::cerr << "Creating service adapter for service '" << ServiceName
            << "' with image '" << imageName << "' using "
            << boost::describe::enum_to_string(Env::ControllerServiceBackend,
                                               "UNKNOWN")
            << " backend." << std::endl;
  switch (Env::ControllerServiceBackend)
  {
  case ServiceAdapterType::KUBERNETES:
  {
    return std::make_unique<KubernetesServiceAdapter>(
        ServiceName, imageName, portMappings,
        KubernetesServiceAdapter::ImagePullPolicy::Never);
  }
  break;
  case ServiceAdapterType::DOCKER:
  {
    return std::make_unique<DockerServiceAdapter>(
        Env::DockerSocketPath, ServiceName, imageName, portMappings);
  }
  break;
  case ServiceAdapterType::INVALID:
    throw std::runtime_error(
        "INVALID service adapter type specified in environment variable "
        "ATLASNET_CONTROLLER_SERVICE_BACKEND");
    break;
  };
  /*  if (std::getenv("KUBERNETES_SERVICE_HOST") ||
       Env::ControllerServiceBackend == ServiceAdapterType::Kubernetes)
   {
     std::cerr << "Detected Kubernetes environment, using "
                  "KubernetesServiceAdapter for "
               << ServiceName << std::endl;
     return std::make_unique<KubernetesServiceAdapter>(
         ServiceName, imageName, std::vector<std::pair<PortType, PortType>>{},
         KubernetesServiceAdapter::ImagePullPolicy::Never);
   }
   else
   {
     std::cerr << "No Kubernetes environment detected, using default "
                  "IServiceAdapter for"
               << ServiceName << std::endl;
     throw std::runtime_error("No suitable service adapter found for " +
                              ServiceName);
   } */
}
