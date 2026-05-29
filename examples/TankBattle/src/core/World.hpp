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

        void Update();
        void Render();
        private:
        std::vector<std::unique_ptr<Entity>> _entities;
    };
};