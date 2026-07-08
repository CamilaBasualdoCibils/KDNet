#pragma once

#include "atlasnet/core/RPC/RPCMacros.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include <utility>
#include <vector>
ATLASNET_RPC(
    EntityLedgerRPC,
    ATLASNET_RPC_METHOD(
        GetEntityInfo,
        ATLASNET_RPC_SIG(
            AtlasNet::Entity::Components::EntityInfo(AtlasNet::AtlasNetEntityID)));
    ATLASNET_RPC_METHOD(
        GetAllEntitiesInfo,
        ATLASNET_RPC_SIG(
            std::unordered_map<AtlasNet::AtlasNetEntityID,
                               AtlasNet::Entity::Components::EntityInfo>())));
