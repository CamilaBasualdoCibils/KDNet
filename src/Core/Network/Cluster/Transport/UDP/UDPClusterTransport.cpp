
#include "AtlasNet/Core/Network/Cluster/Transport/UDP/UDPClusterTransport.hpp"
#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include <fcntl.h>
#include <sys/poll.h>

AtlasNet::Network::Cluster::UDPClusterTransport::UDPBuffer*
AtlasNet::Network::Cluster::UDPClusterTransport::GetFreeBuffer()
{
  if (freeBuffers.empty())
  {
    auto buffer = std::make_unique<UDPBuffer>();
    auto ptr = buffer.get();
    storage.insert(std::move(buffer));
    return ptr;
  }
  else
  {
    auto ptr = freeBuffers.front();
    freeBuffers.pop();
    return ptr;
  }
}
void AtlasNet::Network::Cluster::UDPClusterTransport::ReleaseBuffer(
    AtlasNet::Network::Cluster::UDPClusterTransport::UDPBuffer* buffer)
{
  freeBuffers.push(buffer);
}

AtlasNet::Network::Cluster::UDPClusterTransport::UDPClusterTransport(
    PortType listenPort, std::shared_ptr<IClusterResolver> resolver)
    : IClusterTransport(listenPort, std::move(resolver))
{
  socket_ = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

  if (socket_ < 0)
  {
    throw std::runtime_error("Failed to create UDP socket: " +
                             std::string(std::strerror(errno)));
  }

  // Allow IPv4 mapped addresses too
  int v6Only = 0;
  if (setsockopt(socket_, IPPROTO_IPV6, IPV6_V6ONLY, &v6Only, sizeof(v6Only)) <
      0)
  {
    close(socket_);
    throw std::runtime_error("Failed to disable IPV6_V6ONLY");
  }

  // Allow fast restart after crashes
  int reuse = 1;
  setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in6 addr{};
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(GetListenPort());

  // Bind all interfaces
  addr.sin6_addr = in6addr_any;

  if (::bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
  {
    close(socket_);
    logger->error("Failed to bind UDP socket on port {}: {}",
                  GetListenPort(), std::strerror(errno));
    throw std::runtime_error("Failed to bind UDP socket: " +
                             std::string(std::strerror(errno)));
  }

  // Non-blocking
  int flags = fcntl(socket_, F_GETFL, 0);

  if (flags < 0 || fcntl(socket_, F_SETFL, flags | O_NONBLOCK) < 0)
  {
    close(socket_);

    throw std::runtime_error("Failed to set UDP socket non-blocking");
  }
}

bool AtlasNet::Network::Cluster::UDPClusterTransport::SendMessage(
    const AtlasNetNodeID& destination, std::span<const std::byte> payload)
{
  sockaddr_storage addr{};
  socklen_t addrLen;
  const std::optional<SocketAddress> address =
      GetResolver().ResolveNodeAddress(destination);
  if (!address.has_value())
  {
    logger->error("Failed to resolve node address for destination: {}",
                  destination.to_string());
    return false;
  }
  if (address.value().IsIPv4())
  {
    auto* a = reinterpret_cast<sockaddr_in*>(&addr);

    a->sin_family = AF_INET;
    a->sin_port = htons(address.value().get_port());
    a->sin_addr.s_addr = htonl(address.value().get_ipv4().to_uint32());

    addrLen = sizeof(sockaddr_in);
  }
  else
  {
    auto* a = reinterpret_cast<sockaddr_in6*>(&addr);

    a->sin6_family = AF_INET6;
    a->sin6_port = htons(address.value().get_port());

    auto bytes = address.value().get_ipv6().get_bytes();
    std::memcpy(&a->sin6_addr, bytes.data(), 16);

    addrLen = sizeof(sockaddr_in6);
  }

  ssize_t result = sendto(socket_, payload.data(), payload.size(), 0,
                          reinterpret_cast<sockaddr*>(&addr), addrLen);
  logger->info("Sent UDP message to {} of {} bytes with result {}",
               address.value().to_string(),
               payload.size(), result);

  if (result < 0)
  {
    logger->error("sendto failed: {}", strerror(errno));
  }
  return true;
}

size_t AtlasNet::Network::Cluster::UDPClusterTransport::Receive(
    std::span<ClusterDatagram> packets)
{
        if (!socket_)
    {
      throw std::runtime_error("UDPTransport socket not initialized");
    }
    while (true)
    {
      auto count = TryReceive(packets);

      if (count > 0)
        return count;

      pollfd pfd{};
      pfd.fd = socket_;
      pfd.events = POLLIN;

      poll(&pfd, 1, -1);
    }
}
size_t AtlasNet::Network::Cluster::UDPClusterTransport::TryReceive(
    std::span<ClusterDatagram> packets)
{
  size_t received = 0;
  while (received < packets.size())
  {

    ClusterDatagram& datagram = packets[received];

    sockaddr_storage addr{};
    socklen_t addrLen = sizeof(addr);

    UDPBuffer* buffer = GetFreeBuffer();
    buffer->data.resize(buffer->data.static_capacity);
    ssize_t bytes =
        recvfrom(socket_, buffer->data.data(), buffer->data.static_capacity,
                 MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&addr), &addrLen);
    buffer->data.resize(bytes > 0 ? bytes : 0);
    if (bytes < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;

      logger->error("recvfrom failed: {}", strerror(errno));
      break;
    }
    std::optional<AtlasNetNodeID> nodeID = GetResolver().ResolveNodeID(SocketAddress(reinterpret_cast<sockaddr*>(&addr)));
    if (!nodeID.has_value())
    {
      logger->error("Failed to resolve node ID for source address: {}, dropping packet",
                    SocketAddress(reinterpret_cast<sockaddr*>(&addr)).to_string());
      ReleaseBuffer(buffer);
      continue;
    }
    datagram.source = nodeID.value();
    datagram.payload = std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(buffer->data.data()), bytes);
    datagram.owner = this;
    datagram.userdata = buffer;
    datagram.release = [](void* owner, void* userdata)
    {
      auto* buffer = static_cast<UDPBuffer*>(userdata);
      auto* transport = static_cast<UDPClusterTransport*>(owner);
      transport->ReleaseBuffer(buffer);
    };
    if (addr.ss_family == AF_INET)
    {
      auto* a = reinterpret_cast<sockaddr_in*>(&addr);

      // packet.sourceAddress =
      //     SocketAddress(IPv4(ntohl(a->sin_addr.s_addr)), ntohs(a->sin_port));
    }
    else
    {
      auto* a = reinterpret_cast<sockaddr_in6*>(&addr);

      std::array<uint8_t, 16> ip;
      std::memcpy(ip.data(), &a->sin6_addr, 16);

      // packet.sourceAddress =
      //     SocketAddress(IPv6(ip.data()), ntohs(a->sin6_port));
    }

    ++received;
  }

  return received;
}
