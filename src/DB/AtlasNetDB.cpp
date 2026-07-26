#include "AtlasNetDB.hpp"

#include "valkeymodule.h"

#include <spdlog/spdlog.h>
int AtlasPingCommand(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
{
  ValkeyModule_ReplyWithSimpleString(ctx, "PONG");
  return VALKEYMODULE_OK;
}


extern "C"
{

  int ValkeyModule_OnLoad(ValkeyModuleCtx* ctx, ValkeyModuleString** argv,
                          int argc)
  {

    if (ValkeyModule_Init(ctx, "atlasdb", 1, VALKEYMODULE_APIVER_1) ==
    VALKEYMODULE_ERR)
    {
      return VALKEYMODULE_ERR;
    }
    
    ValkeyModule_Log(ctx, "notice", "AtlasNet Valkey Module loading...");
    AtlasNet::AtlasNetDB db = AtlasNet::AtlasNetDB(0, nullptr);
    db.Run();
    
    if (ValkeyModule_CreateCommand(
            ctx, "atlas.ping",
            [](ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
            { return AtlasPingCommand(ctx, argv, argc); }, "readonly", 0, 0,
            0) == VALKEYMODULE_ERR)
    {
      return VALKEYMODULE_ERR;
    }

    return VALKEYMODULE_OK;
  }
}
void AtlasNet::AtlasNetDB::Initialize() {
   
}
void AtlasNet::AtlasNetDB::Tick() {}
