#pragma once

#include "AtlasNet/Core/Network/Cluster/Transport/IClusterTransport.hpp"
#include <boost/container/static_vector.hpp>
#include <memory>
#include <queue>
#include <unordered_set>
namespace AtlasNet::Network::Cluster
{
class UDPClusterTransport : public IClusterTransport
{
  constexpr static size_t MaxUDPPacketSize = 65536;

  struct UDPBuffer
  {
    boost::container::static_vector<uint8_t, MaxUDPPacketSize> data;
  };
  std::unordered_set<std::unique_ptr<UDPBuffer>> storage;
  std::queue<UDPBuffer*> freeBuffers;
  int socket_;
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("UDPClusterTransport");

  UDPBuffer* GetFreeBuffer();
  void ReleaseBuffer(UDPBuffer* buffer);

public:
  UDPClusterTransport(PortType listenPort,
                      std::shared_ptr<IClusterResolver> resolver);
  bool SendMessage(const AtlasNetNodeID& destination,
                   std::span<const std::byte> payload) override;

  size_t Receive(std::span<ClusterDatagram> packets) override;

  size_t TryReceive(std::span<ClusterDatagram> packets) override;
};
} // namespace AtlasNet::Network::Cluster