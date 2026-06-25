
#include "TankBattleClient.hpp"
#include "Commands.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
int main()
{
  TankBattle::TankBattleClient client;
  client.Run();
  return 0;
}
void TankBattle::TankBattleClient::Run()
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
  std::cerr << "Dispatching PlayerMoveCommand to server." << std::endl;
  PlayerMoveCommand moveCommand;
  moveCommand.delta = {0.01f, 0.0f}; // Example movement delta
  AtlasNetClient_DispatchCommand(moveCommand,
                                 AtlasNet::MessageSendMode::eReliableBatched);
  std::cerr << "Dispatched PlayerMoveCommand to server." << std::endl;
  while (!WindowShouldClose())
  {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Connected to AtlasNet Server!", 190, 200, 20, LIGHTGRAY);

    EndDrawing();
  }
}
