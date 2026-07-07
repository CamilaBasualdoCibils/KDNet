#pragma once 


#include "EntityStream/EntityStreamWebSockController.hpp"
#include "atlasnet/core/node/AtlasNetNode.hpp"
#include <drogon/drogon.h>

#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/collider/Collider.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
namespace AtlasNet
{

class WebBackendService : public IAtlasNetNode
{

  WebBackendService() : IAtlasNetNode(AtlasNetNodeType::WebBackend) {}
  public:
  static WebBackendService& GetInstance()
  {
    static WebBackendService instance;
    return instance;
  }
  using IAtlasNetNode::GetTaskSystem;
  using IAtlasNetNode::GetMessageSystem;
  using IAtlasNetNode::GetRPCSystem;
  using IAtlasNetNode::GetServiceRegistry;
private:
  void OnInit() override;

  void OnShutdown() override;
  void Setup();

private:
  std::optional<EntityStreamWebSockController*> entityStreamWebSock;
};
} // namespace AtlasNet
