#pragma once

#include "atlasnet/core/network/NetworkCommons.hpp"
#include "atlasnet/core/network/address/SocketAddress.hpp"
#include <cstdint>
#include <functional>
#include <span>
namespace AtlasNet::Network{



class IConnection
{
public:
  virtual ~IConnection() = default;

  virtual void Send(std::span<const uint8_t> data, SocketSendMode mode) = 0;
  virtual void Disconnect() = 0;

  virtual void OnPacketArrival(
      std::function<void(IConnection&, std::span<const uint8_t>)> callback) = 0;

  virtual void
  OnStateChange(std::function<void(SocketConnectionState)> callback) = 0;

  virtual SocketAddress RemoteAddress() const = 0;
  virtual SocketConnectionState GetState() const = 0;
};
} // namespace AtlasNet::Network