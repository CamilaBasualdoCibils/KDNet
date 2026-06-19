<!-- @page entity_commands_system AtlasNet Entity Commands -->

@tableofcontents
# AtlasNet Shard Interface

## Overview

AtlasNet shard interface definition. This file defines the `IAtlasNetShard` interface, which serves as the base class for all shards in the AtlasNet system.

It provides standardized functions for:

- entity registration
- serialization / deserialization
- locking mechanisms

This ensures all shards can be managed and communicated with consistently across the AtlasNet ecosystem.

## Interface Definition

The primary interface is `AtlasNet::IAtlasNetShard`.

## Actions

### Registering Entities

Shards can register entities with the AtlasNet core using `RegisterEntity`.  
This allows shards to define entity types managed by the system.
### Client Connect

### Entity Offload

#### Releasing

When transferring an entity, the shard must call `DetachEntity`.  
This ensures the entity is no longer owned locally.

```dot
digraph TransferEntities {
    rankdir=TB;
    layout=neato;
    node [shape=rectangle];
    EntityOOB [label="Entity out of bounds",pos="0,0!"];
    LockEntity [label="LockEntity()",pos="2,0!"];
    SerializeEntity [label="SerializeEntity()",pos="2,-1!"];
    DetachEntity [label="DetachEntity()",pos="0,-1!"];
    UnlockEntity [label="UnlockEntity()",pos="0,-2!"];
    EntityOffloaded [label="Entity offloaded",pos="2,-2!"];

    EntityOOB -> LockEntity;
    LockEntity -> SerializeEntity;
    SerializeEntity -> DetachEntity;
    DetachEntity -> UnlockEntity;
    UnlockEntity -> EntityOffloaded;
}
```
#### Receiving
```dot
digraph ReceiveEntities {
    rankdir=TB;
    layout=neato;
    node [shape=rectangle];
    EntityTransfer [label="Entity Transfer",pos="0,0!"];
    LockEntity [label="LockEntity()",pos="2,0!"];
    DeserializeEntity [label="DeserializeEntity()",pos="2,-1!"];
    UnlockEntity [label="UnlockEntity()",pos="0,-1!"];
    EntityReceived [label="Entity received",pos="0,-2!"];
    EntityTransfer -> LockEntity;
    LockEntity -> DeserializeEntity;
    DeserializeEntity -> UnlockEntity;
    UnlockEntity -> EntityReceived;
}
```
