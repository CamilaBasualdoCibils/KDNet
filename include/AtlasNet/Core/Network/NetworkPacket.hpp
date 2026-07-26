#pragma once
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include <boost/container/small_vector.hpp>
#include <cstdint>
#include <span>
#include <vector>
namespace AtlasNet::Network
{
using PacketPayload = boost::container::small_vector<uint8_t, 128>;
using PacketPayloadView = std::span<const uint8_t>;
struct Packet
{
  PacketPayload payload;
  SocketAddress sourceAddress;
  SocketAddress destinationAddress;
  template <typename archive> void serialize(archive& ar) {
    ar(payload);
  }
};
}; // namespace AtlasNet::Network