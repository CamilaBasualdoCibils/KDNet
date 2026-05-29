#pragma once
#include "atlasnet/core/UUID.hpp"
#include "boost/describe/enum.hpp"
namespace AtlasNet
{
/* struct ContainerIDTag
{
}; */
using ServiceID = UUID;
enum class ServiceType
{
  Controller,
  Shard,
  Proxy,
  WebBackend
};
BOOST_DESCRIBE_ENUM(ServiceType, Controller, Shard, Proxy, WebBackend);
}; // namespace AtlasNet