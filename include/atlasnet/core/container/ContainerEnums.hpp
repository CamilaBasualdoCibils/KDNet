#pragma once
#include "atlasnet/core/UUID.hpp"
#include "boost/describe/enum.hpp"
namespace AtlasNet
{
/* struct ContainerIDTag
{
}; */
using ContainerID = UUID;
enum class ContainerType
{
  Controller,
  Agent,
  Shard,
  Proxy,
  WebBackend
};
BOOST_DESCRIBE_ENUM(ContainerType, Controller, Agent, Shard, Proxy, WebBackend);
}; // namespace AtlasNet