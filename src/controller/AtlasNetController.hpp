
#pragma once
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/Singleton.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/utils/DockerUtils.hpp"
#include "database/internal/InternalDB.hpp"
#include <stdexcept>
namespace AtlasNet
{

class AtlasNetController : public IContainer
{

private:
public:
  AtlasNetController() : IContainer(ContainerType::Controller) {}
  void OnInit() override
  {
    std::cerr << "Initializing AtlasNet Controller..." << std::endl;
  }
  void OnUpdate() override
  {
    
  }
private:
  void OnShutdown() override {
      std::cerr << "Shutting down AtlasNet Controller..." << std::endl;
  }
 
  

 
};
} // namespace AtlasNet
