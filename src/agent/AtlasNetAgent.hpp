

#include "atlasnet/agent/AgentRPC.hpp"
#include "atlasnet/controller/ControllerRPC.hpp"
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/job/JobContext.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "enviroment/Enviroment.hpp"
#include <chrono>
#include <ostream>
#include <thread>

namespace AtlasNet
{

class AtlasNetAgent : public IContainer
{

private:
public:
  AtlasNetAgent() : IContainer(ContainerType::Agent) {}
  void OnInit() override
  {
    std::cerr << "Initializing AtlasNet Agent..." << std::endl;

    GetRPCSystem().Bind<AgentRPC::GetCPUTotalCount>(
        [this]() -> uint32_t
        {
          uint32_t cpuCount = std::thread::hardware_concurrency();
          return cpuCount;
        });
    GetServiceRegistry().RegisterService(ServiceRegistry::ServiceInfo{
        .id = GetContainerID(),
        .address = GetOverlayAddressOfSelf(),
        .containerType = ContainerType::Agent,
        .overlayAddress = GetOverlayAddressOfSelf(),
    });

     DeclareSelfAgentReadyRequest request{
              .agentID = GetContainerID(),
              .agentAddress = GetOverlayAddressOfSelf(),
              .cpuCount = std::thread::hardware_concurrency()};
          GetRPCSystem().Call<ControllerRPC::DeclareSelfAgentReady>(
              SocketAddress(GetControllerAddress(), Env::RPCPort), request);
          std::cerr << "Sent DeclareSelfAgentReady RPC to Controller. Agent "
                       "initialization complete."
                    << std::endl;
    std::cerr << "Agent initialization complete." << std::endl;

    /* SocketAddress controllerAddress(HostName(EnvVars::ControllerHostName),
                                    EnvVars::InternalMessagePort);
    std::cerr << "Agent connecting to Controller at "
              << EnvVars::ControllerHostName << ":"
              << EnvVars::InternalMessagePort << "..." << std::endl;
    JobHandle connectJob =
        GetMessageSystem()
            .Connect(controllerAddress)
            .on_complete(
                [this, controllerAddress](JobContext&)
                {
                  if (GetMessageSystem().IsConnectedTo(controllerAddress))
                  {
                    std::cerr << "Successfully connected to Controller. Agent "
                                 "initialization complete."
                              << std::endl;
                  }
                  else
                  {
                    std::cerr << "Failed to connect to Controller at "
                              << EnvVars::ControllerHostName << ":"
                              << EnvVars::InternalMessagePort
                              << ". Agent initialization failed." << std::endl;
                  }
                },
                JobOpts::Name("Agent Connect to Controller Completion Handler"),
                JobOpts::Notify<JobNotifyLevel::eOnComplete>()); */
  }
  void OnUpdate() override {}

private:
  void OnShutdown() override
  {
    std::cerr << "Shutting down AtlasNet Agent..." << std::endl;
  }
};
} // namespace AtlasNet