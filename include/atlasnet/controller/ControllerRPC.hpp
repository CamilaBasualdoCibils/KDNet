#pragma once

#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/RPC/RPCConcepts.hpp"
#include "atlasnet/core/RPC/RPCMacros.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/shard/shard.hpp"
namespace AtlasNet
{

ATLASNET_RPC(ControllerRPC,
             ATLASNET_RPC_METHOD(GetClosestShardToLocation,
                                 ATLASNET_RPC_SIG(ShardID(Entity::Location))));
} // namespace AtlasNet