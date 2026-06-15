#pragma once
#include "atlasnet/core/UUID.hpp"
#include "boost/describe/enum.hpp"
namespace AtlasNet
{
/* struct ContainerIDTag
{
}; */
using ServiceID = UUID;
enum class ServiceType : uint8_t
{
  Controller = 0,
  Shard = 1,
  Proxy = 2,
  WebBackend = 3,
  Invalid = 4
};
BOOST_DESCRIBE_ENUM(ServiceType, Controller, Shard, Proxy, WebBackend, Invalid);
}; // namespace AtlasNet