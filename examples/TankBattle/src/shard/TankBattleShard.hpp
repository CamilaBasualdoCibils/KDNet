#pragma once
#include "IAtlasNetShard.hpp"
#include "ShardEnums.hpp"
#include "World.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/EntityHandle.hpp"
#include "entities/OrbitEntity.hpp"
#include <atomic>
#include <chrono>
#include <thread>

class TankBattleShard : public AtlasNet::IAtlasNetShard
{
  std::atomic_bool ShouldShutdown{false};
  uint32_t TickSpeed = 20;

public:
  TankBattleShard() = default;
  ~TankBattleShard() override = default;
  TankBattle::World world;
  void OnShardInit() override
  {
    std::cerr << "Starting TankBattleShard..." << std::endl;
    TankBattle::OrbitEntity* orbitEntity =
        world.AddEntity<TankBattle::OrbitEntity>();
    AtlasNet::Entity::Position atlasTransform;
    atlasTransform.Cartesian().position = orbitEntity->transform.position;
    orbitEntity->SetAtlasEntityID(AtlasNet_RegisterEntity(atlasTransform));

    using Clock = std::chrono::steady_clock;
    const auto targetTickTime = TickSpeed > 0
                                    ? std::chrono::duration<double>(
                                          1.0 / static_cast<double>(TickSpeed))
                                    : std::chrono::duration<double>(0.0);

    std::cerr << "Starting main loop of TankBattleShard..." << std::endl;
    while (!ShouldShutdown.load())
    {
      const auto tickStart = Clock::now();
      Time time;
      time.deltaTime =
          std::chrono::duration<double>(Clock::now() - tickStart).count();
      time.totalTime =
          std::chrono::duration<double>(Clock::now().time_since_epoch())
              .count();

      world.Update(time);
      for (auto& entity : world.GetEntities())
      {
        if (entity->GetAtlasEntityID())
        {
          AtlasNet::Entity::Position atlasTransform;
          atlasTransform.Cartesian().position = entity->transform.position;
          AtlasNet_UpdateEntityTransform(entity->GetAtlasEntityID().value(),
                                         atlasTransform);
          /* std::cerr << "Updated transform for entity ID "
                    << entity->GetAtlasEntityID().value().to_string()
                    << std::endl;
          std::cerr << "Entity position: xyz "
                    << glm::to_string(atlasTransform.Cartesian().position)
                    << std::endl; */
        }
      }
      world.Render();
      // std::cerr
      //     << "Tick completed. Delta time: "
      //     << std::chrono::duration<double>(Clock::now() - tickStart).count()
      //     << " seconds." << std::endl;
      const auto tickEnd = Clock::now();
      const auto tickElapsed = tickEnd - tickStart;
      const auto sleepTime = targetTickTime - tickElapsed;

      if (sleepTime > std::chrono::duration<double>(0.0))
      {
        std::this_thread::sleep_for(sleepTime);
      }
    }
  }
  void OnAtlasNetRequest_Shutdown() override
  {
    // Cleanup code for the shard
    std::cerr << "Shutting down AtlasNet Shard..." << std::endl;
    ShouldShutdown.store(true);
  }

  void OnDetachEntity(AtlasNet::EntityDetachState state,
                      const AtlasNet::EntityID& id,
                      const AtlasNet::EntityHandle& remote_handle) override
  {
    // Implementation for detaching an entity from the shard
    std::cerr << "Detaching entity with ID: " << id.to_string() << std::endl;
    // Here you would add logic to remove the entity from any internal data
    // structures and ensure that any references to this entity are properly
    // handled.
  }

  void OnExportEntity(const AtlasNet::EntityID& id,
                      AtlasNet::ByteWriter& writer) override
  {
    // Implementation for serializing an entity's state
    std::cerr << "Exporting entity with ID: " << id.to_string() << std::endl;
    // Here you would add logic to write the entity's state to the ByteWriter
    // This might include writing components, position, health, etc.
  }

  void OnAcquireEntity(const AtlasNet::EntityID& id,
                       AtlasNet::ByteReader& reader) override
  {
    // Implementation for deserializing an entity's state
    std::cerr << "Acquiring entity with ID: " << id.to_string() << std::endl;
    // Here you would add logic to read the entity's state from the ByteReader
    // and reconstruct the entity's components, position, health, etc.
  }

  void OnSpawnClient(const AtlasNet::ClientSpawnInfo& info) override
  {
    // Implementation for spawning a client
    std::cerr << "Spawning client with ID: " << info.clientID.to_string()
              << " and entity ID: " << info.entityID.to_string()
              << " at location " << info.position << std::endl;

    // set position to a random location in xz plane -100,100

    vec3 randomPosition;
    randomPosition.x = static_cast<float>(rand() % 200 - 100);
    randomPosition.y = 0.0f;
    randomPosition.z = static_cast<float>(rand() % 200 - 100);
    AtlasNet::Entity::Position atlasTransform;
    atlasTransform.Cartesian().position = randomPosition;
    AtlasNet_UpdateEntityTransform(info.entityID, atlasTransform);
    // Here you would add logic to handle the client spawn, such as initializing
    // the client's entity in the world and processing any spawnShardPayload
    // data.
  }
};