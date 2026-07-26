#pragma once
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/NetworkPacket.hpp"
#include "AtlasNet/Core/Network/Transport/Datagram/IDatagramTransport.hpp"


namespace AtlasNet::Network
{
class UDPTransport : public IDatagramTransport
{
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("UDPTransport");
  std::optional<int> socket_;

public:
  bool Listen(const SocketAddress& address) override;

  size_t Receive(std::span<Packet> packets) override;

  size_t TryReceive(std::span<Packet> packets) override;

  UDPTransport()
  {

    //
    // Receive thread
    //
  }
  ~UDPTransport();
  void SendMessage(const SocketAddress& address,
                   PacketPayloadView data) override;
  
};
}; // namespace AtlasNet::Network