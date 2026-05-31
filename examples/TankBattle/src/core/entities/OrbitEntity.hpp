#pragma once

#include "Entity.hpp"
#include "Time.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
namespace TankBattle
{
class OrbitEntity : public Entity
{
    
public:
OrbitEntity(World *world) : Entity(world) {}
  void Update(Time time) override
  {
    transform.position.x = 10.0f * glm::cos(time.totalTime);
    transform.position.z = 10.0f * glm::sin(time.totalTime);
    transform.position.y = 10.0f;
  }

  void Render() override
  {
    // Implement rendering logic here
  }
};
}