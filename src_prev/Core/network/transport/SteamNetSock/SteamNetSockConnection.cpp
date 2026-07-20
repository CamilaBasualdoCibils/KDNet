#include "atlasnet/core/network/transport/SteamNetSock/SteamNetSockConnection.hpp"
#include "atlasnet/core/network/transport/SteamNetSock/SteamNetSock.hpp"
AtlasNet::Network::SteamNetSockConnection::SteamNetSockConnection(
    SteamNetSockTransport* transport, HSteamNetConnection handle,
    const SocketAddress& address)
    : steamNetworkingSockets(transport), connectionHandle(handle),
      remoteAddress(address)
{
}
void AtlasNet::Network::SteamNetSockConnection::_ChangeState(SocketConnectionState newState)
{
  state.store(newState, std::memory_order_release);
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