#include "IAtlasNetShard.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/shard/ShardRPC.hpp"

AtlasNet::IAtlasNetShard::IAtlasNetShard()
    : IAtlasNetNode(AtlasNetNodeType::Shard)
{
}
void AtlasNet::IAtlasNetShard::OnInit()
{
  GetLogger()->info("Shard OnInit called.");
  _entityIDGenerator.emplace(GetNodeID());
  _entityLedger.emplace(Entity::EntityLedger::Config{
      .rpcSystem = &GetRPCSystem(),
      .entityIDGenerator = &_entityIDGenerator.value()});

  GetRPCSystem().Bind<ShardRPC::SpawnClient>(
      [this](ShardSpawnClientRequest request)
      { return impl_RPCSpawnClient(request); });

  OnShardInit();
}
AtlasNet::ShardSpawnClientResponse
AtlasNet::IAtlasNetShard::impl_RPCSpawnClient(
    const ShardSpawnClientRequest& request)
{
  // Handle the SpawnClient request here
  GetLogger()->info("Received SpawnClient request for ClientID: {}",
                    request.clientID.to_string());

  EntityID newEntityID;
  {
    Entity::Components::BaseEntityInfo info;
    info.location.worldId = AtlasNet_GetWorldID();
    info.location.position = request.spawnTransform;
    auto writeAccess = _entityLedger->GetWriteAccess();
    newEntityID = writeAccess.CreateEntity(info);
  }

  OnSpawnClient(ClientSpawnInfo{
      .clientID = request.clientID,
      .entityID = newEntityID,
      .position = request.spawnTransform,
      .clientSpawnPayload = std::move(request.clientSpawnPayload),
  });
  return ShardSpawnClientResponse{.entityID = newEntityID};
}

AtlasNet::WorldID AtlasNet::IAtlasNetShard::AtlasNet_GetWorldID()
{
  // Implementation for retrieving the WorldID associated with this shard
  return WorldID();
};
AtlasNet::EntityID
AtlasNet::IAtlasNetShard::AtlasNet_RegisterEntity(Entity::Position transform)
{
  // Implementation for registering a new entity and returning its ID
  assert(_entityLedger.has_value() && "EntityLedger not initialized");
  Entity::Components::BaseEntityInfo info;
  info.location.worldId = AtlasNet_GetWorldID();
  info.location.position = transform;
  auto writeAccess = _entityLedger->GetWriteAccess();

  return writeAccess.CreateEntity(info);
};
void AtlasNet::IAtlasNetShard::AtlasNet_UnregisterEntity(const EntityID& id)
{
  // Implementation for deregistering an existing entity
  assert(_entityLedger.has_value() && "EntityLedger not initialized");
  _entityLedger->GetWriteAccess().RemoveEntity(id);
};
void AtlasNet::IAtlasNetShard::AtlasNet_UpdateEntityTransform(
    const EntityID& id, const Entity::Position& transform)
{
  assert(_entityLedger.has_value() && "EntityLedger not initialized");
  auto writeAccess = _entityLedger->GetWriteAccess();
  auto entityInfo = writeAccess.GetEntityInfo(id);
  entityInfo.baseInfo.location.position = transform;
  writeAccess.SetEntityInfo(id, entityInfo);
};
