#pragma once

#include "atlasnet/core/Singleton.hpp"
#include "atlasnet/core/database/redis/RedisConn.hpp"
#include "enviroment/Enviroment.hpp"
#include <memory>
namespace AtlasNet::Database
{

class InternalDB
{
  std::unique_ptr<RedisConn> Redis;

public:
  InternalDB()
  {
    RedisConn::Settings settings;
    settings.Mode = RedisConn::RedisMode::eStandalone;
    settings.ExceptionOnFailure = true;
    settings.MaxConnectRetries = 5;
    settings.host = EnvVars::InternalDBHost;
    settings.port = EnvVars::InternalDBPort;

    Redis = RedisConn::Connect(settings);
  }

  RedisConn* operator->() const
  {
    return Redis.get();
  }
};

} // namespace AtlasNet