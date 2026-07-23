#include "AtlasNet/Core/Network/Transport/SteamNetSock/SteamNetSockConnection.hpp"
#include "AtlasNet/Core/Network/Transport/SteamNetSock/SteamNetSock.hpp"
AtlasNet::Network::SteamNetSockConnection::SteamNetSockConnection(
    SteamNetSockTransport* transport, HSteamNetConnection handle,
    const SocketAddress& address)
    : steamNetworkingSockets(transport), connectionHandle(handle),
      remoteAddress(address)
{
}
void AtlasNet::Network::SteamNetSockConnection::_ChangeState(
    SocketConnectionState newState)
{
  state.store(newState, std::memory_order_release);
  steamNetworkingSockets->GetLogger()->info(
      "Connection to {} state changed to {}", remoteAddress.to_string(),
      (int)newState);
  if (stateChangeCallback)
  {
    stateChangeCallback(newState);
  }
}
std::shared_ptr<spdlog::logger>
AtlasNet::Network::SteamNetSockConnection::GetLogger() const
{
  return steamNetworkingSockets->GetLogger();
}
void AtlasNet::Network::SteamNetSockConnection::Send(
    std::span<const uint8_t> data, SocketSendMode mode)
{
  steamNetworkingSockets->GetLogger()->info(
      "Sending {} bytes to {}", data.size_bytes(), remoteAddress.to_string());
  // TODO: Implement this pure virtual method.
  /* static_assert(false, "Method `Send` is not implemented."); */
}
void AtlasNet::Network::SteamNetSockConnection::Disconnect()
{
  steamNetworkingSockets->GetLogger()->info("Disconnecting from {}",
                                            remoteAddress.to_string());
  // TODO: Implement this pure virtual method.
  /* static_assert(false, "Method `Disconnect` is not implemented."); */
}
