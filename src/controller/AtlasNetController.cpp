
#include "AtlasNetController.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/universe/WorldConcepts.hpp"
#include "atlasnet/core/universe/WorldEnums.hpp"
#include "boost/describe/enumerators.hpp"

void AtlasNet::AtlasNetController::OnInit()
{
  std::cerr << "Initializing AtlasNet Controller..." << std::endl;

  GetRPCSystem().Bind<ControllerRPC::DeclareSelfAgentReady>(
      [this](const DeclareSelfAgentReadyRequest& request)
      { OnAgentReady(request); });

  GetServiceRegistry().RegisterService(ServiceRegistry::ServiceInfo{
      .id = GetContainerID(),
      .address = GetOverlayAddressOfSelf(),
      .containerType = ContainerType::Controller,
      .overlayAddress = GetOverlayAddressOfSelf(),
  });
  GetGlobalEventSystem().On<WorldCreatedEvent>(
      [this](const WorldCreatedEvent& event) { OnWorldCreated(event); });
  YAML::Node config = YAML::Load(Env::StartupWorlds);
  if (config["worlds"])
  {
    for (const auto& worldNode : config["worlds"])
    {
      WorldDefinition def;
      def.name = worldNode["name"].as<std::string>();
      std::string spaceTypeStr = worldNode["SpaceType"].as<std::string>();

      if (!boost::describe::enum_from_string(spaceTypeStr.c_str(),
                                             def.spaceType))
      {
        std::cerr << "Invalid SpaceType(\"" << spaceTypeStr << "\") for world "
                  << def.name
                  << ", skipping world creation. Must be one of:\n[ "
                  << std::endl;
        // boost describe for each enum?
        boost::mp11::mp_for_each<
            boost::describe::describe_enumerators<WorldSpaceType>>(
            [](auto D) { std::cerr << " - " << D.name << " " << std::endl; });
        std::cerr << "]" << std::endl;
        continue;
      }
      if (def.name.empty())
      {
        std::cerr << "World name cannot be empty, skipping world creation."
                  << std::endl;
        continue;
      }
      auto [result, worldId] = GetUniverse().CreateWorld(def);
      if (result != WorldCreationResult::Success)
      {
        std::cerr << "Failed to create world " << def.name << ": "
                  << boost::describe::enum_to_string(result, "UNKNOWN")
                  << std::endl;
      }
      else
      {
        std::cerr << "Successfully created world " << def.name << " with ID "
                  << worldId->to_string() << std::endl;
      }
    }
  }
}
