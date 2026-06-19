#pragma once
#include <IAtlasNetShard.hpp>

class DummyShard : public AtlasNet::IAtlasNetShard
{
public:
    DummyShard() = default;
    ~DummyShard() override = default;

    void OnShardInit() override
    {
    }
    void OnAtlasNetRequest_Shutdown() override {
        // Cleanup code for the shard
    }

    void OnDetachEntity(const AtlasNet::EntityID& id,
                      const AtlasNet::EntityHandle& remote_handle) override
    {
        // Implementation for detaching an entity from the shard
        // Here you would add logic to remove the entity from any internal data structures
        // and ensure that any references to this entity are properly handled.
    }
    void OnExportEntity(const AtlasNet::EntityID& id, AtlasNet::ByteWriter& writer) override
    {
        // Implementation for serializing an entity's state
        // Here you would add logic to write the entity's state to the ByteWriter
        // This might include writing components, position, health, etc.
    }
    void OnAcquireEntity(const AtlasNet::EntityID& id, AtlasNet::ByteReader& reader) override
    {
        // Implementation for deserializing an entity's state
        // Here you would add logic to read the entity's state from the ByteReader
        // and reconstruct the entity's components, position, health, etc. 
    }
    void OnAtlasNetRequest_LockEntity(const AtlasNet::EntityID& id) override
    {
        // Implementation for locking an entity
        // Here you would add logic to lock the entity for exclusive access
    }
    void OnAtlasNetRequest_UnlockEntity(const AtlasNet::EntityID& id) override
    {
        // Implementation for unlocking an entity
        // Here you would add logic to unlock the entity to allow access by other parts of the system
    }   
    

};