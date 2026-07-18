#pragma once

#include "atlasnet/core/network/transport/IConnection.hpp"
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
  std::atomic<SocketConnectionState> state{SocketConnectionState::eNone};
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

  void Send(std::span<const uint8_t>, SocketSendMode mode) override
  {
    // TODO: Implement this pure virtual method.
    /* static_assert(false, "Method `Send` is not implemented."); */
  }

  void Disconnect() override
  {
    // TODO: Implement this pure virtual method.
    /* static_assert(false, "Method `Disconnect` is not implemented."); */
  }

  void OnPacketArrival(
      std::function<void(IConnection&, std::span<const uint8_t>)> callback)
      override
  {
    // TODO: Implement this pure virtual method.
    /* static_assert(false, "Method `SetPacketCallback` is not implemented.");
     */
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