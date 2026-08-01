
#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Cluster/Channel/IClusterChannel.hpp"
#include "AtlasNet/Core/Network/Cluster/Channel/V1/ClusterChannelV1.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/UDP/UDPClusterTransport.hpp"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
using namespace AtlasNet;
using namespace AtlasNet::Network;
class MockClusterResolver : public Cluster::IClusterResolver
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

TEST(Channels, V1BasicTest)
{
  const AtlasNetNodeID thisNodeID = AtlasNetNodeID::Generate();
  const PortType listenPort = 12345;

  std::shared_ptr<MockClusterResolver> resolver =
      std::make_shared<MockClusterResolver>();

  EXPECT_CALL(*resolver, ResolveNodeAddress(testing::_))
      .WillRepeatedly(testing::Return(
          Network::SocketAddress(Network::IPv6::Loopback(), listenPort)));

  EXPECT_CALL(*resolver,
              ResolveNodeID(testing::A<const Network::SocketAddress&>()))
      .WillRepeatedly(testing::Return(thisNodeID));

  std::shared_ptr<Cluster::IClusterTransport> transport =
      std::make_shared<Cluster::UDPClusterTransport>(listenPort, resolver);

  Cluster::ChannelOptions options{
      .id = 1,
      .transport = transport,
      .delivery = Cluster::DeliveryMode::Reliable,
      .ordering = Cluster::OrderingMode::Ordered,
      .batching = Cluster::BatchMode::Immediate,
      .maxBatchBytes = 1024,
  };
  Cluster::ClusterChannelV1 channel(options);
  std::vector<std::byte> payload = {std::byte{1}, std::byte{2}, std::byte{3},
                                    std::byte{4}, std::byte{5}};
  channel.Send(thisNodeID,
               std::span<const std::byte>(payload.data(), payload.size()));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::array<Cluster::ClusterDatagram, 1> packets;
  size_t received = transport->TryReceive(packets);
  EXPECT_EQ(received, 1);
  EXPECT_EQ(packets[0].payload.size(), payload.size());
  EXPECT_EQ(
      std::memcmp(packets[0].payload.data(), payload.data(), payload.size()),
      0);
}