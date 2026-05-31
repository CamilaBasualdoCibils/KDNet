#pragma once

#include "Entity.hpp"
#include <memory>
#include <vector>
namespace TankBattle
{
class World
{
public:
  World() = default;
  ~World() = default;

  void Update(Time time)
  {
    for (const auto& entity : _entities)
    {
      entity->Update(time);
    }
  }
  void Render() {}

  template <typename T>
    requires std::is_base_of<Entity, T>::value
  T* AddEntity()
  {
    auto entity = std::make_unique<T>(this);
    T* entityPtr = entity.get();
    _entities.push_back(std::move(entity));
    return entityPtr;
  } 
  const std::vector<std::unique_ptr<Entity>>& GetEntities() const
  {
    return _entities;
  }

private:
  std::vector<std::unique_ptr<Entity>> _entities;
};
}; // namespace TankBattle