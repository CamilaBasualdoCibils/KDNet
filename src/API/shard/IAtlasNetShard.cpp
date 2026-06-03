#include "IAtlasNetShard.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"

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
}
