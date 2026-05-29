#pragma once

namespace TankBattle
{
class Entity
{
public:
  Entity() = default;
  virtual ~Entity() = default;
  virtual void Update() = 0;
  virtual void Render() = 0;
};
} // namespace TankBattle