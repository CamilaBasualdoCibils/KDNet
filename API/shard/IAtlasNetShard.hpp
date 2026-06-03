

#pragma once

#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/EntityHandle.hpp"
#include "atlasnet/core/entity/EntityLedger.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "atlasnet/core/universe/WorldConcepts.hpp"
#include <iostream>
namespace AtlasNet
{
/**
 * @brief The IAtlasNetShard interface defines the required functions that any
 * shard must implement to interact with the AtlasNet core. This includes
 * entity registration, serialization, deserialization, and locking mechanisms.
 * The interface ensures consistent management and communication of shards
 * within the AtlasNet ecosystem.
 */
class IAtlasNetShard : IService
{
  std::optional<Entity::EntityLedger> _entityLedger;

public:
  IAtlasNetShard();
  virtual ~IAtlasNetShard() = default;

private:
  void OnInit() override;
  
  void OnShutdown() override
  {
    std::cerr << "Shard OnShutdown called." << std::endl;
    _entityLedger.reset();
  }

public:
  
  WorldID AtlasNet_GetWorldID()
  {
    // Implementation for retrieving the WorldID associated with this shard
    return WorldID();
  };
  /**
   * @brief Registers a new entity created by the shard and returns its
   * EntityID. This should be called whenever the shard creates a new entity
   * that needs to be tracked by the AtlasNet system, so that the AtlasNet
   * system can track it and manage its lifecycle.
   *
   * @return EntityID new ID which must be kept by the shard and used for all
   * future references to this entity. The shard is responsible for ensuring the
   * uniqueness of this ID across all entities it creates, and for using this ID
   * in all subsequent interactions with the AtlasNet system regarding this
   * entity.
   */
  EntityID AtlasNet_RegisterEntity(Entity::Transform transform)
  {
    // Implementation for registering a new entity and returning its ID
    assert(_entityLedger.has_value() && "EntityLedger not initialized");
    Entity::Components::BaseEntityInfo info;
    info.location.worldId = AtlasNet_GetWorldID();
    info.location.transform = transform;
    auto writeAccess = _entityLedger->GetWriteAccess();

    return writeAccess.CreateEntity(info);
  };
  /**
   * @brief Deregister an existing local entity from the AtlasNet system. This
   * should be called when the entity in question should no longer be
   * transferable or tracked by the AtlasNet system, such as a local only entity
   * or when an entity is being destroyed.
   * @param id
   */
  void AtlasNet_UnregisterEntity(const EntityID& id)
  {
    // Implementation for deregistering an existing entity
    assert(_entityLedger.has_value() && "EntityLedger not initialized");
    _entityLedger->GetWriteAccess().RemoveEntity(id);
  };
  void AtlasNet_UpdateEntityTransform(const EntityID& id,
                                      const Entity::Transform& transform)
  {
    assert(_entityLedger.has_value() && "EntityLedger not initialized");
    auto writeAccess = _entityLedger->GetWriteAccess();
    auto entityInfo = writeAccess.GetEntityInfo(id);
    entityInfo.baseInfo.location.transform = transform;
    writeAccess.SetEntityInfo(id, entityInfo);
  };
  virtual void OnAtlasNetRequest_Shutdown() = 0; // Pure virtual function to be
                                                 // implemented by
  // derived classes

  /**
   * @brief When this function is called, the shard should release the entity
   * with the given ID and remote handle. This indicates that the entity is no
   * longer being managed by this shard and should be released from any
   * internal tracking or resources.
   *
   * @param id the ID of the entity to be released. The shard should ensure that
   * any necessary cleanup is performed for this entity, and that it is properly
   * removed from any internal data structures or systems that were managing it.
   * The shard should also ensure that any references to this entity are
   * properly handled to prevent dangling pointers or other issues.
   * @param remote_handle the remote handle associated with the entity
   */
  virtual void
  OnAtlasNetRequest_DetachEntity(const EntityID& id,
                                 const EntityHandle& remote_handle) = 0;
  /**
   * @brief When this function is called, the shard should serialize the entity
   * with the given ID and write its data to the provided ByteWriter. This
   * allows the AtlasNet system to capture the current state of the entity for
   * purposes such as saving, networking, or transferring between shards. The
   * shard is responsible for ensuring that all relevant data about the entity
   * is included in the serialization process, and that it is written in a
   * format that can be correctly deserialized later.
   *
   * @param id
   * @param writer
   */
  virtual void OnAtlasNetRequest_SerializeEntity(const EntityID& id,
                                                 ByteWriter& writer) = 0;

  /**
   * @brief When this function is called, the shard should deserialize the
   * entity with the given ID using the provided ByteWriter. This allows the
   * AtlasNet system to restore the state of the entity from previously captured
   * data.
   *
   * @param id
   * @param reader
   */
  virtual void OnAtlasNetRequest_DeserializeEntity(const EntityID& id,
                                                   ByteReader& reader) = 0;

  /**
   * @brief When this function is called, the shard should lock the entity with
   * the given ID. This indicates that the entity is currently being modified or
   * accessed in a way that requires exclusive access. The shard should ensure
   * that any necessary synchronization mechanisms are in place to prevent
   * concurrent modifications to the entity while it is locked. The shard should
   * also ensure that the entity is properly unlocked when the exclusive access
   * is no longer needed, to allow other parts of the system to access or modify
   * the entity as necessary.
   *
   * @param id
   */
  virtual void OnAtlasNetRequest_LockEntity(const EntityID& id) = 0;

  /**
   * @brief When this function is called, the shard should unlock the entity
   * with the given ID. This indicates that the entity is no longer being
   * modified or accessed in a way that requires exclusive access. The shard
   * should ensure that any necessary synchronization mechanisms are in place to
   * allow other parts of the system to access or modify the entity as
   * necessary.
   *
   * @param id
   */
  virtual void OnAtlasNetRequest_UnlockEntity(const EntityID& id) = 0;
};
} // namespace AtlasNet
