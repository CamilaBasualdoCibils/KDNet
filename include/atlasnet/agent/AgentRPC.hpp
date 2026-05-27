#pragma once

#include "atlasnet/core/RPC/RPCConcepts.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"

namespace AtlasNet
{
ATLASNET_RPC(AgentRPC, ATLASNET_RPC_METHOD(GetCPUTotalCount, uint32_t);
             ATLASNET_RPC_METHOD(GetCPUAvailCount, uint32_t);
             ATLASNET_RPC_METHOD(SpawnShard, void);)

};
