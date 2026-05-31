#pragma once
#include "atlasnet/core/shard/shard.hpp"
#include "boost-src/libs/container/include/boost/container/small_vector.hpp"
#include "boost-src/libs/static_string/include/boost/static_string/static_string.hpp"
#include <atlasnet/core/entity/Entity.hpp>
#include <cstdint>
#include <string>
namespace AtlasNet
{
struct ICommand
{
  boost::static_string<64> commandName;

  EntityID targetEntity;
  uint64_t logical_entity_sequence;

  std::variant<ShardID, ClientID> sender;
};
struct PayloadCommand : public ICommand
{
  boost::container::small_vector<uint8_t, 64> payload;
};
} // namespace AtlasNet
