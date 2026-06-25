#pragma once

#include "IAtlasNetClient.hpp"
#include "raylib.h"
#include <iostream>
namespace TankBattle
{
class TankBattleClient : public AtlasNet::IAtlasNetClient
{
public:
  TankBattleClient() = default;
  ~TankBattleClient() = default;
  void Run();
};
} // namespace TankBattle