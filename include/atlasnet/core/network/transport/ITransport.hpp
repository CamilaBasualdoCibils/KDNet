#pragma once

#include "atlasnet/core/network/address/SocketAddress.hpp"
#include <cstdint>
#include <span>
namespace AtlasNet::Network
{

class IConnection;

class ConnectionRequest;
class IListener;

class ITransport
{
public:
  virtual ~ITransport() = default;

  virtual std::shared_ptr<IListener> Listen(const SocketAddress&) = 0;

  virtual std::shared_ptr<IConnection> Connect(const SocketAddress&) = 0;
};
} // namespace AtlasNet