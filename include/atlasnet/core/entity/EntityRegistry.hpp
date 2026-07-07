#pragma once

#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/entity/Entity.hpp"

#include "enviroment/Enviroment.hpp"
namespace AtlasNet
{
class EntityRegistry
{
public:
  void SetEntityToShard(const EntityID& id, const AtlasNetShardID& shardId)
  {
    _redisConn->HashMap().GetSet().HSet(EntityID2ShardIDMapKey, id.to_string(),
                                        shardId.to_string());
  }

  std::optional<AtlasNetShardID> GetShardForEntity(const EntityID& id)
  {
    auto result = _redisConn->HashMap().GetSet().HGet(EntityID2ShardIDMapKey,
                                                      id.to_string());
    if (result)
    {
      return AtlasNetShardID::from_string(*result);
    }
    return std::nullopt;
  }
  void RemoveEntity(const EntityID& id)
  {
    _redisConn->HashMap().Delete().HDel(EntityID2ShardIDMapKey, id.to_string());
  }

private:
  Database::RedisConn* _redisConn;
  const std::string EntityRegistryPrefix = Env::DatabaseNamespace + "Entity:";
  const std::string EntityID2ShardIDMapKey =
      EntityRegistryPrefix + "EntityID2ShardIDMap";
};
}; // namespace AtlasNet