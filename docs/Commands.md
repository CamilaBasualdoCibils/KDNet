<!-- @page entity_commands_system AtlasNet Entity Commands -->

@tableofcontents

# AtlasNet Entity Commands

## overview Overview

AtlasNet entity commands system. This page describes the command-based
architecture for managing entities in the AtlasNet ecosystem. It covers:
- Command definitions
- Command processing flow
- Examples of common commands
This system allows for flexible and extensible management of entities across
different shards and components in AtlasNet.  
Commands can be sent by Clients or other shards to perform operations on entities. 
The definition of commands is up to the implementation of the shard.  

### Client To Shard logic

an example of a simple command requesting an entity to move to a new position:

```dot
digraph ClientToShardCommand {
    rankdir=LR;
    node [shape=rectangle];
    Client [label="Client"];

    AtlasNet [label="AtlasNet"];
    subgraph cluster_Shard {
        label="Owning Shard";
        CommandHandler [label="Command Handler",shape=rectangle];
        GameLogic [label="Game Logic"];
    }


    Client -> AtlasNet [label="Send Command"];
    AtlasNet -> CommandHandler [label="Forward Command"];
    CommandHandler -> GameLogic [label="Process Command"];

    GameLogic -> CommandHandler [label="Send Signal"]
    CommandHandler -> AtlasNet [label="Forward Signal"]
    AtlasNet -> Client [label="Process Signal"]

}
```

### Shard to Shard logic

In a shard-to-shard command scenario, one shard may need to request an operation on an entity that is owned by another shard. For example, if Shard A needs to update the state of an entity owned by Shard B, it can send a command to Shard B through AtlasNet.

```dot
digraph ShardToShardCommand {
    rankdir=LR;
    subgraph cluster_ShardA {
        label="Shard A";

        EntityHandle [label="Entity Handle",shape=rectangle];
        Command [label="cmd example.UpdateEntity"];
        Signal [label="signal example.EntityUpdated"];

    }
   subgraph cluster_ShardB {
        label="Shard B";
        CommandHandler [label="Command Handler",shape=rectangle];
        GameLogic [label="Game Logic"];

    }
    Signal -> EntityHandle [label="Call Signal"]
    Command -> EntityHandle [label="Call Command"];

    AtlasNet [label="AtlasNet",shape=box];
    EntityHandle -> AtlasNet [label="Dispatch Signal",shape=rectangle]
    EntityHandle -> AtlasNet [label="Dispatch Command",shape=rectangle];
    AtlasNet -> CommandHandler [label="Forward Command"];
    Client [label="Client"];
    AtlasNet -> Client [label="Forward Signal"]
    CommandHandler -> GameLogic [label="Process Command"];
}
```

In the event that the entity in question is in the same shard that dispatches the command, the command can be processed directly without needing to go through AtlasNet. This allows for efficient handling of commands that do not require cross-shard communication.

```dot
digraph ShardToSelfCommand {
    rankdir=LR;
    node [shape=rectangle];
    subgraph cluster_Shard {
        label="Shard A";
            GameLogic [label="GameLogic"];
            EntityHandle [label="Entity Handle"];
            Command [label="example.LocalCommand"];
    }

GameLogic -> Command [label="Call Command"];
Command -> EntityHandle [label="Process Command"];
EntityHandle -> GameLogic [label="Update State"];
}
```