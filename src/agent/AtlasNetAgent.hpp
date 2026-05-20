

#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "enviroment/Enviroment.hpp"
#include <chrono>
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

    SocketAddress controllerAddress(HostName(EnvVars::ControllerServiceName),
                                    EnvVars::InternalMessagePort);
    std::cerr << "Agent connecting to Controller at "
              << EnvVars::ControllerServiceName << ":"
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
                              << EnvVars::ControllerServiceName << ":"
                              << EnvVars::InternalMessagePort
                              << ". Agent initialization failed." << std::endl;
                  }
                },
                JobOpts::Name("Agent Connect to Controller Completion Handler"),
                JobOpts::Notify<JobNotifyLevel::eOnComplete>());
  }
  void OnUpdate() override {}

private:
  void OnShutdown() override
  {
    std::cerr << "Shutting down AtlasNet Agent..." << std::endl;
  }
};
} // namespace AtlasNet