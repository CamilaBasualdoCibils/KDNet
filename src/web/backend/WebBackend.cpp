
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

class WebBackend : public IService
{
public:
  WebBackend() : IService(ServiceType::WebBackend) {}

private:
  void OnInit() override
  {
    std::cerr << "Initializing AtlasNet Web Backend..." << std::endl;
    // Initialize and start the Drogon HTTP server
    drogon::app().addListener("0.0.0.0", 2000);
    Setup();
    drogon::app().run();
  }
  void OnUpdate() override
  {
    // The main loop can be used for periodic tasks if needed
  }
  void OnShutdown() override
  {
    std::cerr << "Shutting down AtlasNet Web Backend..." << std::endl;
    // Perform any necessary cleanup before shutdown
    drogon::app().quit();
  }
  void Setup()
  {
    drogon::app().registerHandler(
        "/api/entity-fetch",
        [this](const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        { HandleEntityFetchRequest(req, std::move(callback)); });
  }
  void HandleEntityFetchRequest(
      const drogon::HttpRequestPtr& req,
      std::function<void(const drogon::HttpResponsePtr&)>&& callback)
  {
    std::cerr << "Received entity fetch request from "
              << req->getPeerAddr().toIp().c_str() << std::endl;
    // Parse incoming JSON body
    auto jsonPtr = req->getJsonObject();

    _Json response;
    response["entities"] = _Json::array();
    {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<float> dis(-100.0f, 100.0f);
      for (int i = 0; i < 10; i++)
      {

        Entity::Entity_Serializable entity;
        entity.entityInfo.emplace();
        entity.entityInfo->id = (EntityID)EntityID::Generate();
        
        //entity.entityInfo->location.transform.position =
        //    vec3(dis(gen), dis(gen), dis(gen));
        entity.entityInfo->baseInfo.location.worldId = WorldID::Generate();
        entity.colliderInfo.emplace();
        if (i % 2 == 0)
        {
          entity.colliderInfo->collider = BoxCollider(vec3(1.0f, 1.0f, 1.0f));
        }
        else
        {
          entity.colliderInfo->collider = SphereCollider(1.0f);
        }
        _Json entityJson;
        entity.to_json(entityJson);
        response["entities"].push_back(entityJson);
      }
    }
    std::cerr << "Sending response: " << response.dump() << std::endl;
    // Handle the entity fetch request and send a response
    auto httpResponse = drogon::HttpResponse::newHttpResponse();
    httpResponse->setStatusCode(drogon::k200OK);
    httpResponse->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    httpResponse->setBody(response.dump());
    callback(httpResponse);
  }
};
} // namespace AtlasNet
int main(int argc, char** argv)
{

  AtlasNet::WebBackend webBackend;
  webBackend.Init();
  return 0;
}