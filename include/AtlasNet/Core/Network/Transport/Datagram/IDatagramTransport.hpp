#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/NetworkPacket.hpp"
#include "AtlasNet/Core/Network/Transport/Datagram/DatagramCommons.hpp"
#include "AtlasNet/Core/Network/Transport/ITransport.hpp"
#include "AtlasNet/Core/Network/Transport/TransportCommons.hpp"
#include <cstdint>
#include <span>
#include <vector>
namespace AtlasNet::Network
{
class IDatagramTransport : public ITransport
{
public:
  virtual ~IDatagramTransport() = default;

  virtual void SendMessage(const SocketAddress& address, PacketPayloadView data) = 0;
  virtual bool Listen(const SocketAddress& address) = 0;
  virtual size_t Receive(std::span<Packet> packets) = 0;

  virtual size_t TryReceive(std::span<Packet> packets) = 0;
};
} // namespace AtlasNet::Network