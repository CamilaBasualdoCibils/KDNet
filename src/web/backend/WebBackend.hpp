#pragma once 


#include "EntityStream/EntityStreamWebSockController.hpp"
#include "atlasnet/core/container/Container.hpp"
#include <drogon/drogon.h>

#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/collider/Collider.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
namespace AtlasNet
{

class WebBackendService : public IService
{

  WebBackendService() : IService(ServiceType::WebBackend) {}
  public:
  static WebBackendService& GetInstance()
  {
    static WebBackendService instance;
    return instance;
  }
  using IService::GetJobSystem;
  using IService::GetMessageSystem;
  using IService::GetRPCSystem;
  using IService::GetServiceRegistry;
private:
  void OnInit() override;

  void OnShutdown() override;
  void Setup();
  void HandleEntityFetchRequest(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
  std::optional<EntityStreamWebSockController*> entityStreamWebSock;
};
} // namespace AtlasNet
