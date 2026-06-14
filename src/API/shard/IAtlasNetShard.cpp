#include "IAtlasNetShard.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/shard/ShardRPC.hpp"

AtlasNet::IAtlasNetShard::IAtlasNetShard() : IService(ServiceType::Shard)
{
  Init();
}
void AtlasNet::IAtlasNetShard::OnInit()
{
  std::cerr << "Shard OnInit called." << std::endl;
  _entityLedger.emplace(
      Entity::EntityLedger::Config{.rpcSystem = &GetRPCSystem()});

  GetServiceRegistry().RegisterService({
      .id = GetContainerID(),
      .address = GetHostName(),
      .containerType = ServiceType::Shard,
  });

  GetRPCSystem().Bind<ShardRPC::SpawnClient>(
      [this](ShardSpawnClientRequest request)
      {
        // Handle the SpawnClient request here
        std::cerr << "Received SpawnClient request for ClientID: "
                  << request.clientID.to_string() << std::endl;
        return ShardSpawnClientResponse{};
      });
}
