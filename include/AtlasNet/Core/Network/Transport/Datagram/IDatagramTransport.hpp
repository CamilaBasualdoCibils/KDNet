#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Packet/NetPacket.hpp"
#include "AtlasNet/Core/Network/NodeAddressResolver.hpp"
#include "AtlasNet/Core/Network/Transport/Datagram/DatagramCommons.hpp"
#include "AtlasNet/Core/Network/Transport/DatagramBuffer.hpp"
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
  IDatagramTransport(std::shared_ptr<INodeAddressResolver> resolver)
      : addressResolver(std::move(resolver))
  {
  }
  virtual ~IDatagramTransport() = default;

  virtual void SendMessage(const AtlasNetNodeID& node,
                           const PacketPayloadView& data) = 0;
  virtual bool Listen() = 0;
  virtual size_t Receive(std::span<DatagramBuffer> packets) = 0;

  virtual size_t TryReceive(std::span<DatagramBuffer> packets) = 0;

  INodeAddressResolver& GetAddressResolver() const
  {
    return *addressResolver.get();
  }

private:
  std::shared_ptr<INodeAddressResolver> addressResolver;
};
} // namespace AtlasNet::Network