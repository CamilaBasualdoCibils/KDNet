#pragma once

#include "src/Node/ServiceDiscovery/IServiceDiscovery.hpp"
#include <algorithm>
#include <memory>
#include <sw/redis++/async_redis++.h>
#include <sw/redis++/async_subscriber.h>
#include <sw/redis++/redis++.h>
#include <sw/redis++/subscriber.h>
namespace AtlasNet::Service
{
class RedisServiceLease : public IServiceLease
{
public:
  RedisServiceLease(std::shared_ptr<sw::redis::Redis> redisClient,
                    const std::string& serviceKey)
      : redisClient(redisClient), serviceKey(serviceKey)
  {
  }
  void Renew() override;
  void Release() override;

private:
  std::shared_ptr<sw::redis::Redis> redisClient;
  std::string serviceKey;
};
class RedisServiceDiscovery : public IServiceDiscovery
{
public:
  RedisServiceDiscovery(std::shared_ptr<sw::redis::Redis> redisClient,
                        std::shared_ptr<sw::redis::AsyncRedis> redisAsyncClient)
      : redisClient(redisClient), redisAsyncClient(redisAsyncClient),
        redisSubscriber(redisAsyncClient->subscriber())
  {
    redisSubscriber.on_message(
        [this](const std::string& channel, const std::string& message)
        { HandleServiceRegistryUpdate(message); });
    redisSubscriber.subscribe(
        std::format("__keyspace@0__:{}", ServiceRegistryKey));
  }
  ~RedisServiceDiscovery()
  {
    redisSubscriber.unsubscribe(
        std::format("__keyspace@0__:{}", ServiceRegistryKey));
  }
  std::unique_ptr<IServiceLease>
  RegisterService(const ServiceData& data) override
  {
  }

private:
  void HandleServiceRegistryUpdate(const std::string& message)
  {
    logger->info("Received service registry update: {}", message);
    std::string operation = message;
    std::transform(operation.begin(), operation.end(), operation.begin(),
                   ::tolower);
    if (operation == "hset")
    {
      logger->info("New Service Registered");
    }
    else if (operation == "hexpired")
    {
      logger->info("Service Heartbeat ");
    }
    
  }
  std::shared_ptr<sw::redis::Redis> redisClient;
  std::shared_ptr<sw::redis::AsyncRedis> redisAsyncClient;
  sw::redis::AsyncSubscriber redisSubscriber;
  constexpr static std::string_view ServiceRegistryKey = "ServiceRegistry";
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("RedisServiceDiscovery");
};
} // namespace AtlasNet::Service