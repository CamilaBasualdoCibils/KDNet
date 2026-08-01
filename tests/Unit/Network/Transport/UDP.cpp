
#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/MacAddress.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/ClusterDatagram.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/IClusterResolver.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/UDP/UDPClusterTransport.hpp"
#include "Commons.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
using namespace AtlasNet;

class MockClusterResolver : public Network::Cluster::IClusterResolver
{
public:
  MOCK_METHOD(std::optional<Network::SocketAddress>, ResolveNodeAddress,
              (AtlasNetNodeID nodeID), (override));
  MOCK_METHOD(std::optional<Network::MACAddress>, ResolveNodeMAC,
              (AtlasNetNodeID nodeID), (override));
  MOCK_METHOD(std::optional<AtlasNetNodeID>, ResolveNodeID,
              (const Network::SocketAddress& address), (override));
  MOCK_METHOD(std::optional<AtlasNetNodeID>, ResolveNodeID,
              (const Network::MACAddress& mac), (override));
};
TEST(UDP, Init)
{
  std::shared_ptr<MockClusterResolver> resolver =
      std::make_shared<MockClusterResolver>();
  Network::Cluster::UDPClusterTransport transport(12345, resolver);
}

TEST(UDP, SendReceive)
{
  std::shared_ptr<MockClusterResolver> resolver =
      std::make_shared<MockClusterResolver>();
  AtlasNetNodeID thisNodeID = AtlasNetNodeID::Generate();
  Network::PortType listenPort = pick_available_port();
  Network::Cluster::UDPClusterTransport transport(listenPort, resolver);
  std::vector<std::byte> payload = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}};
  EXPECT_CALL(*resolver, ResolveNodeAddress(testing::_))
      .WillRepeatedly(testing::Return(
          Network::SocketAddress(Network::IPv6::Loopback(), listenPort)));
  EXPECT_CALL(*resolver,
              ResolveNodeID(testing::A<const Network::SocketAddress&>()))
      .WillRepeatedly(testing::Return(thisNodeID));

  transport.SendMessage(AtlasNetNodeID::Generate(), payload);

  std::array<Network::Cluster::ClusterDatagram, 1> packets;
  size_t received = transport.Receive(packets);
  ASSERT_EQ(received, 1);
  ASSERT_EQ(packets[0].payload.size(), payload.size());
  ASSERT_EQ(std::memcmp(packets[0].payload.data(), payload.data(), payload.size()),
            0);
  for (const auto& datagram : packets)
  {
    spdlog::log(spdlog::level::info, "Received datagram from {} with {} bytes",
                datagram.source.to_string(), datagram.payload.size());
  }
  size_t receivedAgain = transport.TryReceive(packets);
  ASSERT_EQ(receivedAgain, 0);
  packets[0].Release();
}
