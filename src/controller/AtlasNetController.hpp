
#pragma once
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/Singleton.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "database/internal/InternalDB.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include <stdexcept>
namespace AtlasNet
{

class AtlasNetController : public IContainer
{

private:


public:
  AtlasNetController() : IContainer(ContainerType::Controller) {}
  void OnInit() override {

  }

private:
  void OnShutdown() override {}

};
} // namespace AtlasNet
