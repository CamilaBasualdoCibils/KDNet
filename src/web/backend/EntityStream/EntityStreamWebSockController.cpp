#include "EntityStreamWebSockController.hpp"
#include "WebBackend.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/EntityLedgerRPC.hpp"
#include "atlasnet/core/job/JobContext.hpp"
#include "enviroment/Enviroment.hpp"
#include <algorithm>
#include <execution>
#include <future>
#include <iostream>
#include <numeric>

void EntityStreamWebSockController::handleNewMessage(
    const WebSocketConnectionPtr& wsConnPtr, std::string&& message,
    const WebSocketMessageType& conn)
{
  // write your application logic here
  std::cerr << "Received message from WebSocket client: " << message
            << std::endl;

  // wsConnPtr->send(message);
}
void EntityStreamWebSockController::handleNewConnection(
    const HttpRequestPtr& req, const WebSocketConnectionPtr& wsConnPtr)
{
  auto state = std::make_shared<ConnectionState>();
  /*
    // ---- Parse query params ----
    auto shardParam = req->getParameter("Shard");
    if (!shardParam.empty())
    {
      state->filter.shardId = static_cast<uint32_t>(std::stoul(shardParam));
    }

    auto entityData = req->getParameter("EntityData");
    // Example: EntityData=Transform,Velocity
    if (!entityData.empty())
    {
      std::stringstream ss(entityData);
      std::string item;
      while (std::getline(ss, item, ','))
      {
        state->filter.components.insert(item);
      }
    } */
  wsConnPtr->setContext(state);

  /*   LOG_INFO << "EntityStreamWebSock connected with filter - Shard: "
             << (state->filter.shardId ? std::to_string(*state->filter.shardId)
                                       : "None")
             << ", Components: "
             << (state->filter.components.empty()
                     ? "None"
                     : std::accumulate(
                           state->filter.components.begin(),
                           state->filter.components.end(), std::string(),
                           [](const std::string& a, const std::string& b)
                           { return a + (a.empty() ? "" : ",") + b; }));
                            */
  LOG_INFO << "EntityStreamWebSock connected with new connection.";
  std::cerr << "New WebSocket connection established from "
            << req->getPeerAddr().toIp().c_str() << std::endl;
  std::unique_lock lock(ConnectionMutex);
  connectionStates[wsConnPtr] = state;

  if (!FetchJobRunning.load())
  {
    LOG_INFO
        << "Starting entity data fetch job as this is the first connection.";
    StartFetchJob();
  }
  // write your application logic here
}
void EntityStreamWebSockController::handleConnectionClosed(
    const WebSocketConnectionPtr& wsConnPtr)
{
  // write your application logic here
  std::cerr << "WebSocket connection closed." << std::endl;
  std::unique_lock lock(ConnectionMutex);
  connectionStates.erase(wsConnPtr);

  if (connectionStates.empty())
  {
    LOG_INFO
        << "No more WebSocket connections. Stopping entity data fetch job.";
    FetchJobShouldShutdown.store(true);
    if (fetchEntityDataJob.valid())
    {
      fetchEntityDataJob.wait();
    }
    FetchJobRunning.store(false);
    FetchJobShouldShutdown.store(false);
  }
}
EntityStreamWebSockController::EntityStreamWebSockController() {}

void EntityStreamWebSockController::StartFetchJob()
{
  assert((!fetchEntityDataJob.valid() || !FetchJobRunning.load()) &&
         "Fetch job is already running");

  fetchEntityDataJob =
      AtlasNet::WebBackendService::GetInstance().GetJobSystem().Submit(
          [this](AtlasNet::JobContext& ctx)
          {
            auto& backend = AtlasNet::WebBackendService::GetInstance();
            auto& rpcSystem = backend.GetRPCSystem();

            std::vector<AtlasNet::ServiceRegistry::ServiceInfo> shardServices;
            backend.GetServiceRegistry().GetServicesOfType(
                AtlasNet::ServiceType::Shard, shardServices);

            std::cerr << "Fetched " << shardServices.size()
                      << " shard services\n";

            // -----------------------------
            // 1. Dispatch all RPC calls FIRST (no waiting yet)
            // -----------------------------
            using ResultType =
                std::unordered_map<AtlasNet::EntityID,
                                   AtlasNet::Entity::Components::EntityInfo>;

            std::vector<std::future<ResultType>> futures;
            futures.reserve(shardServices.size());

            for (const auto& info : shardServices)
            {
              std::cerr << "Dispatching shard " << info.id.to_string() << " at "
                        << info.address.to_string() << "\n";

              futures.push_back(
                  rpcSystem.Call<EntityLedgerRPC::GetAllEntitiesInfo>(
                      AtlasNet::SocketAddress(
                          info.address, AtlasNet::Env::InternalMessagePort)));
            }

            // -----------------------------
            // 2. Collect results (single-threaded, safe .get())
            // -----------------------------
            std::unordered_map<AtlasNet::EntityID,
                               AtlasNet::Entity::Components::EntityInfo>
                entityInfoCache;

            for (auto& fut : futures)
            {
              if (fut.wait_for(std::chrono::seconds(1)) ==
                  std::future_status::ready)
              {
                try
                {
                  auto result = fut.get();

                  std::cerr << "Fetched " << result.size()
                            << " entities from shard\n";

                  for (auto& [entityId, entityInfo] : result)
                  {
                    std::cerr
                        << "Entity ID: " << entityId.to_string()
                        << "\npos: " << entityInfo.baseInfo.location.position
                        << std::endl;
                    entityInfoCache.emplace(entityId, entityInfo);
                  }
                }
                catch (const std::exception& e)
                {
                  std::cerr << "Shard RPC failed: " << e.what() << "\n";
                }
              }
              else
              {
                std::cerr << "Shard RPC timeout\n";
              }
            }

            // -----------------------------
            // 3. Broadcast to all connections (snapshot first!)
            // -----------------------------
            std::vector<WebSocketConnectionPtr> conns;
            {
              std::lock_guard lock(ConnectionMutex);
              for (auto& [ws, _] : connectionStates)
                conns.push_back(ws);
            }

            _Json payload;

            for (const auto& [entityId, entityInfo] : entityInfoCache)
            {
              _Json entityJson;
              entityInfo.to_json(entityJson);
              payload[entityId.to_string()] = entityJson;
            }

            const std::string message = payload.dump();
            std::cerr << "EntityStream Response json\n"
                      << payload.dump(2) << std::endl;
            for (auto& ws : conns)
            {
              ws->send(message);
            }

            // -----------------------------
            // 4. Reschedule
            // -----------------------------
            if (!FetchJobShouldShutdown.load())
            {
              ctx.set_repeat_once(std::chrono::milliseconds(50));
            }
          });

  FetchJobRunning.store(true);
}