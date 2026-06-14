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
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Connecting to AtlasNet Server", 190, 200, 20, LIGHTGRAY);
    EndDrawing();
    AtlasNet::IAtlasNetClient::AtlasNetClientError error;
    std::string errorMessage;
    if (!AtlasNetClient_Connect("172.17.0.1", 42000, &error, &errorMessage))
    {
      std::cerr << "Failed to connect to AtlasNet server: " << errorMessage
                << std::endl;
      throw std::runtime_error("Failed to connect to AtlasNet server: " +
                               errorMessage);
      return;
    }
    else
    {
      std::cerr << "Successfully connected to AtlasNet server." << std::endl;
    }

    while (!WindowShouldClose())
    {
      BeginDrawing();
      ClearBackground(RAYWHITE);
      DrawText("Connected to AtlasNet Server!", 190, 200, 20, LIGHTGRAY);
      EndDrawing();
    }
  }
};
} // namespace TankBattle