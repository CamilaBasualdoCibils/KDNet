#pragma once

#include "AtlasNet/Core/Network/RPC/NetworkTransportRPC.hpp"
#include "AtlasNet/Core/Service/AtlasNetService.hpp"
#include "AtlasNet/DB/Handshake.hpp"
#include "valkeymodule.h"
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
namespace AtlasNet
{
class AtlasNetDB : public AtlasNetService
{
  ValkeyModuleCtx* g_ctx = nullptr;

public:
  AtlasNetDB(ValkeyModuleCtx* ctx, int argc, char** argv)
      : AtlasNetService(AtlasNetServiceType::DB, argc, argv)
  {
    g_ctx = ctx;
  }

private:
  void Initialize() override;
  void Tick() override;

  ValkeyModuleCtx* GetValkeyCtx()
  {
    return g_ctx;
  }
  void RegisterHandshakeBinds()
  {
    
    GetHandshakeRPC().Bind<DB::RPC_DB_Ping>(
        [this](const Network::RPC::NetworkTransportRPC::CallContext&,
               int) -> int
        {
          GetLogger()->info("Received ping request");
          return 0;
        });
    GetHandshakeRPC().Bind<DB::RPC_DB_RegisterNode>(
        [this](
            const Network::RPC::NetworkTransportRPC::CallContext&,
            const DB::RegisterNodeRequest& request) -> DB::RegisterNodeResponse
        { return RegisterNode(request); });
  }

  DB::RegisterNodeResponse RegisterNode(const DB::RegisterNodeRequest& request)
  {
    ValkeyModule_ThreadSafeContextLock(GetValkeyCtx());

    DB::RegisterNodeResponse response;
    GetLogger()->info("Registering node with ID: ");

    ValkeyModule_ThreadSafeContextUnlock(GetValkeyCtx());
    return response;
  }
};
}; // namespace AtlasNet