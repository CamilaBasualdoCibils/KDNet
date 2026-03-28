#pragma once

#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include <cstdint>
#include <cstdlib>
namespace AtlasNet
{

class EnvVars
{

  static inline const char* GetEnvVarOrDefault(const char* varName,
                                               const char* defaultValue)
  {
    const char* value = std::getenv(varName);
    return value ? value : defaultValue;
  }

public:
  const static inline uint32_t TickRate =
      std::atoi(GetEnvVarOrDefault("ATLASNET_TICK_RATE", "20"));

  const static inline PortType RPCPort = static_cast<PortType>(
      std::atoi(GetEnvVarOrDefault("ATLASNET_RPC_PORT", "41001")));

  const static inline HostAddress InternalDBHost =
      HostAddress(GetEnvVarOrDefault("ATLASNET_INTERNAL_DB_HOST", "127.0.0.1"));
  const static inline PortType InternalDBPort = static_cast<PortType>(
      std::atoi(GetEnvVarOrDefault("ATLASNET_INTERNAL_DB_PORT", "6379")));
  const static inline std::string OverlayNetworkName =
      GetEnvVarOrDefault("ATLASNET_NETWORK_NAME", "atlasnet_net");
  const static inline std::string StackName =
      GetEnvVarOrDefault("ATLASNET_STACK_NAME", "atlasnet");
};

} // namespace AtlasNet