#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/ClusterDatagram.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/IClusterResolver.hpp"

namespace AtlasNet::Network::Cluster
{
class IClusterTransport
{
public:
  IClusterTransport(PortType listenPort,
                    std::shared_ptr<IClusterResolver> resolver)
      : listenPort(listenPort), nodeResolver(std::move(resolver))
  {
  }
  virtual ~IClusterTransport() = default;
  virtual bool SendMessage(const AtlasNetNodeID& destination,
                           std::span<const std::byte> payload) = 0;

  virtual size_t Receive(std::span<ClusterDatagram> packets) = 0;

  virtual size_t TryReceive(std::span<ClusterDatagram> packets) = 0;

  PortType GetListenPort() const
  {
    return listenPort;
  }

protected:
  IClusterResolver& GetResolver() const
  {
    return *nodeResolver;
  }

private:
  PortType listenPort;
  std::shared_ptr<IClusterResolver> nodeResolver;
};
} // namespace AtlasNet::Network::Cluster