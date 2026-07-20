#pragma once

#include "AtlasNet/Core/Network/Transport/IConnection.hpp"
#include "spdlog/logger.h"
#include <atomic>
namespace AtlasNet::Network
{

class SteamNetSockTransport;
class SteamNetSockListener;
class SteamNetSockConnection;

class SteamNetSockConnection : public IConnection
{
  friend class SteamNetSockTransport;

  SteamNetSockTransport* steamNetworkingSockets;
  HSteamNetConnection connectionHandle;
  SocketAddress remoteAddress;
  std::atomic<SocketConnectionState> state{SocketConnectionState::None};
  std::function<void(IConnection&, std::span<const uint8_t>)> packetCallback;
  std::function<void(SocketConnectionState)> stateChangeCallback;

protected:
  void _ChangeState(SocketConnectionState newState);

private:
  std::shared_ptr<spdlog::logger> GetLogger() const;

public:
  SocketConnectionState GetState() const override
  {
    // TODO: Implement this pure virtual method.
    return state.load(std::memory_order_acquire);
  }

  void OnStateChange(std::function<void(SocketConnectionState)> callback) override
  {
    stateChangeCallback = callback;
    callback(state.load(std::memory_order_acquire));
  }

  void Send(std::span<const uint8_t> data, SocketSendMode mode) override;

  void Disconnect() override;

  void OnPacketArrival(
      std::function<void(IConnection&, std::span<const uint8_t>)> callback)
      override
  {
    packetCallback = callback; 
    // TODO: Implement this pure virtual method.
    /* static_assert(false, "Method `SetPacketCallback` is not implemented."); */
  }

  SocketAddress RemoteAddress() const override
  {
    // TODO: Implement this pure virtual method.
    /* static_assert(false, "Method `RemoteAddress` is not implemented."); */
    return remoteAddress;
  }

  SteamNetSockConnection(SteamNetSockTransport* transport,
                         HSteamNetConnection handle,
                         const SocketAddress& address);
};
} // namespace AtlasNet::Network