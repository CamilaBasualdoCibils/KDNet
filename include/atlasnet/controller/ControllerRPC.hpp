#pragma once

#include "atlasnet/core/address/Address.hpp"
#include "atlasnet/core/RPC/RPCConcepts.hpp"
#include "atlasnet/core/RPC/RPCMacros.hpp"

#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"

namespace AtlasNet
{

ATLASNET_RPC(ControllerRPC,
             ATLASNET_RPC_METHOD(GetClosestShardToLocation,
                                 ATLASNET_RPC_SIG(AtlasNetShardID(Entity::Location))));
} // namespace AtlasNet