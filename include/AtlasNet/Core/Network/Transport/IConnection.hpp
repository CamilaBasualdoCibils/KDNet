#pragma once

#include "AtlasNet/Core/Network/Transport/TransportCommons.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
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