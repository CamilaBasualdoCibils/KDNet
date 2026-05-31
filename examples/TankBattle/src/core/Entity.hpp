#pragma once

#include "Time.hpp"

#include "atlasnet/core/entity/Entity.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <type_traits>
namespace TankBattle
{
  class World;
class Transform
{
public:
  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;

  glm::mat4 GetModelMatrix() const
  {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model *= glm::mat4_cast(rotation);
    model = glm::scale(model, scale);
    return model;
  }
};
class Entity
{
  World* const world;
  std::optional<AtlasNet::EntityID> AtlasEntityID;
public:
  Transform transform;
  Entity(World* world) : world(world) {}
  virtual ~Entity() = default;
  virtual void Update(Time time) = 0;
  virtual void Render() = 0;
  World* GetWorld() const
  {
    return world;
  }

  void SetAtlasEntityID(const AtlasNet::EntityID& id)
  {
    AtlasEntityID = id;
  }
  std::optional<AtlasNet::EntityID> GetAtlasEntityID() const
  {
    return AtlasEntityID;
  }
  void ClearAtlasEntityID()
  {
    AtlasEntityID.reset();
  }

};
} // namespace TankBattle