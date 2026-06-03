#pragma once
#include "IAtlasNetShard.hpp"
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
  void Run()
  {
    TankBattle::OrbitEntity* orbitEntity =
        world.AddEntity<TankBattle::OrbitEntity>();
    AtlasNet::Entity::Transform atlasTransform;
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
          AtlasNet::Entity::Transform atlasTransform;
          atlasTransform.Cartesian().position = entity->transform.position;
          AtlasNet_UpdateEntityTransform(entity->GetAtlasEntityID().value(),
                                         atlasTransform);
        }
      }
      world.Render();
      //std::cerr
      //    << "Tick completed. Delta time: "
      //    << std::chrono::duration<double>(Clock::now() - tickStart).count()
      //    << " seconds." << std::endl;
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

  void OnAtlasNetRequest_DetachEntity(
      const AtlasNet::EntityID& id,
      const AtlasNet::EntityHandle& remote_handle) override
  {
    // Implementation for detaching an entity from the shard
    std::cerr << "Detaching entity with ID: " << id.to_string() << std::endl;
    // Here you would add logic to remove the entity from any internal data
    // structures and ensure that any references to this entity are properly
    // handled.
  }

  void OnAtlasNetRequest_SerializeEntity(const AtlasNet::EntityID& id,
                                         AtlasNet::ByteWriter& writer) override
  {
    // Implementation for serializing an entity's state
    std::cerr << "Serializing entity with ID: " << id.to_string() << std::endl;
    // Here you would add logic to write the entity's state to the ByteWriter
    // This might include writing components, position, health, etc.
  }

  void
  OnAtlasNetRequest_DeserializeEntity(const AtlasNet::EntityID& id,
                                      AtlasNet::ByteReader& reader) override
  {
    // Implementation for deserializing an entity's state
    std::cerr << "Deserializing entity with ID: " << id.to_string()
              << std::endl;
    // Here you would add logic to read the entity's state from the ByteReader
    // and reconstruct the entity's components, position, health, etc.
  }
  void OnAtlasNetRequest_LockEntity(const AtlasNet::EntityID& id) override
  {
    // Implementation for locking an entity
    std::cerr << "Locking entity with ID: " << id.to_string() << std::endl;
    // Here you would add logic to lock the entity for exclusive access
  }
  void OnAtlasNetRequest_UnlockEntity(const AtlasNet::EntityID& id) override
  {
    // Implementation for unlocking an entity
    std::cerr << "Unlocking entity with ID: " << id.to_string() << std::endl;
    // Here you would add logic to unlock the entity to allow access by other
    // parts of the system
  }
};