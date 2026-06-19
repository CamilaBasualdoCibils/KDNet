#include "atlasnet/core/entity/EntityLedger.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/entity/EntityLedgerRPC.hpp"
#include <unordered_map>

void AtlasNet::Entity::EntityLedger::SetRPCBinds()
{
  rpcSystem->Bind<EntityLedgerRPC::GetAllEntitiesInfo>(
      [this]()
      {
        std::cerr << "RPC call received: GetAllEntitiesInfo" << std::endl;
        std::unordered_map<EntityID, Components::EntityInfo> allInfo;
        {
          auto access = GetReadAccess();
          std::cerr << "Entity Count: " << IDMapping.size() << std::endl;
          for (const auto& [id, enttId] : IDMapping.left)
          {
            const auto& entityInfo = access.GetEntityInfo(id);
            allInfo[id] = entityInfo;
            std::cerr << "Entity ID: " << id.to_string() <<"\npos: " << entityInfo.baseInfo.location.position << std::endl;
          }
        }
        return allInfo;
      });
}