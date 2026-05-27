#pragma once
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/container/Container.hpp"
namespace AtlasNet
{
template <ContainerType Type> class ContainerHandle
{
  ContainerID id;
  SocketAddress address;

public:
  ContainerHandle(const ContainerID& id, const SocketAddress& address)
      : id(id), address(address)
  {
  }
  const ContainerID& GetID() const
  {
    return id;
  }
  const SocketAddress& GetAddress() const
  {
    return address;
  }
};
} // namespace AtlasNet