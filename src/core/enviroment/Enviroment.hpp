#pragma once

#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include <cstdint>
#include <cstdlib>
namespace AtlasNet
{

class Env
{

  static inline const char* GetEnvVarOrDefault(const char* varName,
                                               const char* defaultValue)
  {
    const char* value = std::getenv(varName);
    return value ? value : defaultValue;
  }

public:
  const static inline std::string StackName =
      GetEnvVarOrDefault("ATLASNET_STACK_NAME", "DefaultStackName");
  const static inline uint32_t TickRate =
      std::atoi(GetEnvVarOrDefault("ATLASNET_TICK_RATE", "20"));

  const static inline PortType RPCPort = static_cast<PortType>(
      std::atoi(GetEnvVarOrDefault("ATLASNET_RPC_PORT", "41001")));

  const static inline std::string NetworkName =
      GetEnvVarOrDefault("ATLASNET_NETWORK_NAME", "atlasnet_net");
  const static inline std::string NetworkSubnet =
      GetEnvVarOrDefault("ATLASNET_NETWORK_SUBNET", "10.0.0.0/16");

  const static inline std::string DatabaseHostName =
      GetEnvVarOrDefault("ATLASNET_DATABASE_HOST_NAME", "Atlasnet-Database");

  const static inline std::string DatabaseNamespace =
      GetEnvVarOrDefault("ATLASNET_DATABASE_NAMESPACE", "atlasnet:");
  const static inline PortType DatabasePort = static_cast<PortType>(
      std::atoi(GetEnvVarOrDefault("ATLASNET_DATABASE_PORT", "6379")));

  const static inline PortType InternalMessagePort = static_cast<PortType>(
      std::atoi(GetEnvVarOrDefault("ATLASNET_INTERNAL_MESSAGE_PORT", "41000")));
  const static inline bool DebugMode =
      std::atoi(GetEnvVarOrDefault("ATLASNET_DEBUG_MODE", "1")) != 0;

  const static inline std::string StartupWorlds =
      GetEnvVarOrDefault("ATLASNET_WORLDS", R"(
worlds:
  - name: "Main"
    SpaceType: "Cartesian2D")");

  const static inline std::string ShardImageName =
      GetEnvVarOrDefault("ATLASNET_SHARD_IMAGE_NAME", "atlasnet/shard:latest");
  const static inline uint32_t ShardCPUReserve = static_cast<uint32_t>(
      std::atoi(GetEnvVarOrDefault("ATLASNET_SHARD_CPU_RESERVE", "4")));
};

} // namespace AtlasNet