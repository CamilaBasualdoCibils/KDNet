
#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/NetworkPacket.hpp"
#include "AtlasNet/Core/Network/Transport/Datagram/UDP/UDPTransport.hpp"
#include "AtlasNet/Core/Network/Transport/DatagramBuffer.hpp"
#include <gtest/gtest.h>
using namespace AtlasNet::Network;
TEST(UDP, Init)
{
  UDPTransport transport;
}
TEST(UDP, Listen)
{
  UDPTransport transport;
  SocketAddress address(IPv4(127, 0, 0, 1), 12345);
  ASSERT_TRUE(transport.Listen(address));
}
TEST(UDP, SendReceive)
{
  UDPTransport transport;
  SocketAddress address(IPv4(127, 0, 0, 1), 12345);
  ASSERT_TRUE(transport.Listen(address));

  PacketPayload payload = {1, 2, 3, 4, 5};

  transport.SendMessage(address, payload);

  std::array<DatagramBuffer, 1> packets;
  size_t received = transport.Receive(packets);
  ASSERT_EQ(received, 1);
  ASSERT_EQ(packets[0].data.size(), payload.size());
  ASSERT_EQ(std::memcmp(packets[0].data.data(), payload.data(),
                        payload.size()),
            0);

  size_t receivedAgain = transport.TryReceive(packets);
  ASSERT_EQ(receivedAgain, 0);
  packets[0].Release();
}
