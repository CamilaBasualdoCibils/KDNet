#include "atlasnet/core/network/transport/SteamNetSock/SteamNetSockListener.hpp"
#include "atlasnet/core/network/transport/SteamNetSock/SteamNetSock.hpp"
void AtlasNet::Network::SteamNetSockListener::SetConnectionRequestCallback(
    std::function<void(ConnectionRequest&, IListener&)> callback)
{
  connectionRequestCallback = callback;
}
void AtlasNet::Network::SteamNetSockListener::Close()
{
  // TODO: Implement this pure virtual method.
  /* static_assert(false, "Method `Close` is not implemented."); */
}
AtlasNet::Network::SteamNetSockListener::SteamNetSockListener(
    SteamNetSockTransport* transport, HSteamListenSocket socket,
    const SocketAddress& address)
    : steamNetworkingSockets(transport), listenSocket(socket),
      remoteAddress(address)
{
}
auto AtlasNet::Network::SteamNetSockListener::GetLogger() const
{
  return steamNetworkingSockets->GetLogger();
}