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
  void Run()
  {
    InitWindow(800, 600, "Tank Battle");
    SetTargetFPS(120);

    AtlasNetClient_Init();
    AtlasNet::IAtlasNetClient::AtlasNetClientError error;
    std::string errorMessage;
    if (!AtlasNetClient_Connect("localhost", 8080, &error, &errorMessage))
    {
      std::cerr << "Failed to connect to AtlasNet server: " << errorMessage
                << std::endl;
      throw std::runtime_error("Failed to connect to AtlasNet server: " +
                               errorMessage);
      return;
    }
  }
};
} // namespace TankBattle