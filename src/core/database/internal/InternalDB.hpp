#pragma once

#include "atlasnet/core/Singleton.hpp"
#include "atlasnet/core/database/redis/RedisConn.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "enviroment/Enviroment.hpp"
#include <format>
#include <iostream>
#include <memory>
namespace AtlasNet::Database
{

class InternalDB : public Singleton<InternalDB>
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

    std::cerr << std::format("Attempting to connect to InternalDB at {}:{} in "
                             "{} mode with up to {} retries...",
                             settings.host.to_string(), settings.port,
                             boost::describe::enum_to_string(settings.Mode,
                                                             "UNKNOWN MODE"),
                             settings.MaxConnectRetries)
              << std::endl;
    Redis = RedisConn::Connect(settings);
    if (!Redis)
    {
      throw std::runtime_error(std::format(
          "Failed to connect to InternalDB at {}:{} after {} attempts",
          settings.host.to_string(), settings.port,
          settings.MaxConnectRetries));
    }
    std::cerr
        << std::format(
               "Successfully connected to InternalDB at {}:{} in {} mode.",
               settings.host.to_string(), settings.port,
               boost::describe::enum_to_string(settings.Mode, "UNKNOWN MODE"))
        << std::endl;
  }

  bool IsConnected() const
  {
    return Redis != nullptr;
  }
  RedisConn* operator->() const
  {
    return Redis.get();
  }
};

} // namespace AtlasNet::Database