#pragma once

#include "AtlasNet/Core/Network/Cluster/Channel/ClusterMessage.hpp"
#include "AtlasNet/Core/Network/Cluster/Channel/IClusterChannel.hpp"
#include "AtlasNet/Core/Network/Cluster/Transport/ClusterDatagram.hpp"
#include "AtlasNet/Core/Serialization/NetBinarySerializer.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
namespace AtlasNet::Network::Cluster
{
namespace ChannelV1
{
constexpr uint32_t CLUSTER_CHANNEL_V1_MAGIC = 0x43484E31; // "CHN1"
constexpr uint8_t CLUSTER_CHANNEL_V1_VERSION = 1;
enum class PacketFlags : uint8_t
{
  None = 0,
  Reliable = 1 << 0,
  AckOnly = 1 << 1,
};
constexpr PacketFlags operator|(PacketFlags lhs, PacketFlags rhs) noexcept
{
  return static_cast<PacketFlags>(static_cast<uint8_t>(lhs) |
                                  static_cast<uint8_t>(rhs));
}

constexpr bool HasFlag(PacketFlags value, PacketFlags flag) noexcept
{
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}
// for the packet
struct PacketHeader
{
  uint32_t magic = CLUSTER_CHANNEL_V1_MAGIC;
  uint8_t version = 1;
  PacketFlags flags = PacketFlags::None;
  ChannelID channel = 0;
  uint64_t packetSequence = 0;
  uint64_t ackSequence = 0;
  uint64_t ackBits = 0;
  uint64_t messageCount = 0;
  uint16_t payloadBytes = 0;
  template <typename Archive> void serialize(Archive& archive)
  {
    archive(magic, version, flags, channel, packetSequence, ackSequence,
            ackBits, messageCount, payloadBytes);
  }
  constexpr static size_t NetSize() noexcept
  {
    NetBinaryWriter writer;
    writer(PacketHeader());
    return writer.GetBytes().size();
  }
};
// for each message
struct MessageHeader
{
  uint64_t messageSequence = 0;
  uint16_t payloadBytes = 0;
  template <typename Archive> void serialize(Archive& archive)
  {
    archive(messageSequence, payloadBytes);
  }
  constexpr static size_t NetSize() noexcept
  {
    NetBinaryWriter writer;
    writer(MessageHeader());
    return writer.GetBytes().size();
  }
};

} // namespace ChannelV1
class ClusterChannelV1 : public IClusterChannel
{
public:
  ClusterChannelV1(const ChannelOptions& options) : IClusterChannel(options) {}

  bool Send(const AtlasNetNodeID& destination,
            std::span<const std::byte> payload) override
  {
    if (payload.empty())
    {
      assert(false && "Cannot send empty payload");
      return false;
    }

    const ChannelOptions& options = GetOptions();

    // Account for the per-message framing too.
    const size_t framedSize =
        ChannelV1::MessageHeader::NetSize() + payload.size();

    if (framedSize > options.maxBatchBytes)
    {
      assert(false && "Message too large for channel batch size");
      // Fragmentation would be required here.
      return false;
    }

    std::scoped_lock lock(mutex_);

    ProcessTimers();

    PeerState& peer = peers_[destination];

    QueuedMessage message;

    if (options.ordering != OrderingMode::Unordered ||
        options.delivery == DeliveryMode::Reliable)
    {
      message.sequence = peer.send.nextMessageSequence++;
    }

    message.payload.assign(payload.begin(), payload.end());

    peer.send.queuedBytes += framedSize;
    peer.send.queuedMessages.push_back(std::move(message));

    switch (options.batching)
    {
    case BatchMode::Immediate:
      return FlushPeer(destination, peer);

    case BatchMode::Automatic:
      if (peer.send.queuedBytes >= options.maxBatchBytes ||
          peer.send.queuedMessages.size() >= options.maxQueuedMessages)
      {
        return FlushPeer(destination, peer);
      }

      return true;

    case BatchMode::Manual:
      return true;
    }

    return false;
  }

  void Flush() override
  {
    std::scoped_lock lock(mutex_);

    ProcessTimers();

    for (auto& [destination, peer] : peers_)
      FlushPeer(destination, peer);

    FlushPendingAcks();
  }

  size_t Receive(std::span<ClusterMessage> messages) override
  {
    if (messages.empty())
      return 0;

    for (;;)
    {
      {
        std::scoped_lock lock(mutex_);

        ProcessTimers();

        const size_t count = CopyReadyMessages(messages);

        if (count != 0)
          return count;
      }

      // Do not hold mutex_ while blocking inside the transport.
      PumpTransport(true);
    }
  }

  size_t TryReceive(std::span<ClusterMessage> messages) override
  {
    if (messages.empty())
      return 0;

    std::scoped_lock lock(mutex_);

    ProcessTimers();

    size_t count = CopyReadyMessages(messages);

    if (count == messages.size())
      return count;

    PumpTransport(false);
    FlushPendingAcks();

    count += CopyReadyMessages(messages.subspan(count));
    return count;
  }

private:
  using Clock = std::chrono::steady_clock;

  struct QueuedMessage
  {
    uint64_t sequence = 0;
    std::vector<std::byte> payload;
  };

  struct PendingPacket
  {
    uint64_t packetSequence = 0;
    std::vector<std::byte> bytes;

    Clock::time_point lastSent;
    uint32_t attempts = 0;
  };

  struct SendState
  {
    uint64_t nextPacketSequence = 1;
    uint64_t nextMessageSequence = 1;

    size_t queuedBytes = 0;
    std::deque<QueuedMessage> queuedMessages;

    // Packets waiting for an ACK.
    std::map<uint64_t, PendingPacket> pendingPackets;
  };

  struct BufferedMessage
  {
    uint64_t sequence = 0;
    std::shared_ptr<const std::vector<std::byte>> storage;
    size_t offset = 0;
    size_t size = 0;
  };

  struct ReceiveState
  {
    // Packet-level ACK state.
    uint64_t highestPacketSequence = 0;
    uint64_t receivedPacketBits = 0;

    // Message-level ordering state.
    uint64_t nextOrderedSequence = 1;
    uint64_t highestSequencedMessage = 0;

    std::map<uint64_t, BufferedMessage> reorderBuffer;

    bool ackPending = false;
  };

  struct PeerState
  {
    SendState send;
    ReceiveState receive;
  };

  struct ReadyMessage
  {
    AtlasNetNodeID source;
    uint64_t sequence = 0;

    std::shared_ptr<const std::vector<std::byte>> storage;
    size_t offset = 0;
    size_t size = 0;
  };

  std::unordered_map<AtlasNetNodeID, PeerState> peers_;
  std::deque<ReadyMessage> readyMessages_;

  std::mutex mutex_;

  static constexpr auto RetransmissionTimeout = std::chrono::milliseconds(100);

  static constexpr uint32_t MaxRetransmissions = 10;

  bool FlushPeer(const AtlasNetNodeID& destination, PeerState& peer)
  {
    auto& queue = peer.send.queuedMessages;

    if (queue.empty())
      return true;

    bool success = true;

    while (!queue.empty())
    {
      std::vector<QueuedMessage> packetMessages;
      size_t packetBytes = ChannelV1::PacketHeader ::NetSize();

      while (!queue.empty())
      {
        const QueuedMessage& next = queue.front();

        const size_t framedSize =
            ChannelV1::MessageHeader::NetSize() + next.payload.size();

        if (!packetMessages.empty() &&
            packetBytes + framedSize > GetOptions().maxBatchBytes)
        {
          break;
        }

        if (packetBytes + framedSize > GetOptions().maxBatchBytes)
        {
          // This message requires fragmentation.
          success = false;
          queue.pop_front();
          continue;
        }

        packetBytes += framedSize;
        packetMessages.push_back(std::move(queue.front()));
        queue.pop_front();
      }

      if (packetMessages.empty())
        continue;

      uint64_t packetSequence = 0;

      if (GetOptions().delivery == DeliveryMode::Reliable)
        packetSequence = peer.send.nextPacketSequence++;

      std::vector<std::byte> bytes =
          BuildPacket(peer, packetMessages, packetSequence, false);

      for (const QueuedMessage& message : packetMessages)
      {
        peer.send.queuedBytes -=
            ChannelV1::MessageHeader::NetSize() + message.payload.size();
      }

      SendPacket(destination, peer, std::move(bytes), packetSequence);
    }

    return success;
  }
  void FlushPendingAcks()
  {
    for (auto& [destination, peer] : peers_)
    {
      if (!peer.receive.ackPending)
        continue;

      std::vector<std::byte> bytes = BuildPacket(peer, {}, 0, true);

      GetOptions().transport->SendMessage(destination, bytes);
    }
  }
  void ProcessTimers()
  {
    if (GetOptions().delivery != DeliveryMode::Reliable)
      return;

    const Clock::time_point now = Clock::now();

    for (auto& [destination, peer] : peers_)
    {
      auto& pendingPackets = peer.send.pendingPackets;

      for (auto iterator = pendingPackets.begin();
           iterator != pendingPackets.end();)
      {
        PendingPacket& packet = iterator->second;

        if (now - packet.lastSent < RetransmissionTimeout)
        {
          ++iterator;
          continue;
        }

        if (packet.attempts >= MaxRetransmissions)
        {
          // Report peer/channel failure through an event or callback.
          iterator = pendingPackets.erase(iterator);
          continue;
        }

        GetOptions().transport->SendMessage(destination, packet.bytes);

        packet.lastSent = now;
        ++packet.attempts;
        ++iterator;
      }
    }
  }

  size_t PumpTransport(bool blocking)
  {
    constexpr size_t MaxDatagramsPerPump = 64;

    std::array<ClusterDatagram, MaxDatagramsPerPump> datagrams;

    IClusterTransport& transport = *GetOptions().transport;

    const size_t received = blocking ? transport.Receive(datagrams)
                                     : transport.TryReceive(datagrams);

    if (received == 0)
      return 0;

    std::scoped_lock lock(mutex_);

    for (size_t index = 0; index < received; ++index)
    {
      ClusterDatagram& datagram = datagrams[index];

      ProcessDatagram(datagram);

      datagram.Release();
    }

    return received;
  }
  void ProcessDatagram(const ClusterDatagram& datagram)
  {
    auto storage = std::make_shared<std::vector<std::byte>>(
        datagram.payload.begin(), datagram.payload.end());

    NetBinaryReader reader(*storage);

    ChannelV1::PacketHeader header;
    header.serialize(reader);

    if (header.magic != ChannelV1::CLUSTER_CHANNEL_V1_MAGIC ||
        header.version != ChannelV1::CLUSTER_CHANNEL_V1_VERSION ||
        header.channel != GetOptions().id)
    {
      return;
    }

    PeerState& peer = peers_[datagram.source];

    ProcessAcknowledgements(peer.send, header.ackSequence, header.ackBits);

    const bool reliable =
        ChannelV1::HasFlag(header.flags, ChannelV1::PacketFlags::Reliable);

    if (reliable)
    {
      const bool isNew =
          RegisterReceivedPacket(peer.receive, header.packetSequence);

      if (!isNew)
      {
        // Still send/piggyback an ACK, but do not redeliver.
        return;
      }
    }

    if (ChannelV1::HasFlag(header.flags, ChannelV1::PacketFlags::AckOnly))
    {
      return;
    }

    if (header.messageCount > GetOptions().maxQueuedMessages)
      return;

    for (uint16_t index = 0; index < header.messageCount; ++index)
    {
      ChannelV1::MessageHeader messageHeader;
      messageHeader.serialize(reader);

      if (messageHeader.payloadBytes > reader.Remaining())
        return;

      const size_t payloadOffset = reader.Position();

      ProcessMessage(datagram.source, peer.receive,
                     messageHeader.messageSequence, storage, payloadOffset,
                     messageHeader.payloadBytes);

      reader.Skip(messageHeader.payloadBytes);
    }
  }

  void ProcessAcknowledgements(SendState& state, uint64_t ackSequence,
                               uint64_t ackBits)
  {
    if (ackSequence == 0)
      return;

    state.pendingPackets.erase(ackSequence);

    for (uint64_t index = 0; index < 64; ++index)
    {
      if ((ackBits & (uint64_t{1} << index)) == 0)
        continue;

      if (ackSequence <= index + 1)
        continue;

      const uint64_t acknowledged = ackSequence - index - 1;

      state.pendingPackets.erase(acknowledged);
    }
  }

  bool RegisterReceivedPacket(ReceiveState& state, uint64_t packetSequence)
  {
    if (packetSequence == 0)
      return true;

    if (state.highestPacketSequence == 0)
    {
      state.highestPacketSequence = packetSequence;
      state.receivedPacketBits = 0;
      state.ackPending = true;
      return true;
    }

    if (packetSequence > state.highestPacketSequence)
    {
      const uint64_t difference = packetSequence - state.highestPacketSequence;

      if (difference >= 64)
      {
        state.receivedPacketBits = 0;
      }
      else
      {
        state.receivedPacketBits <<= difference;

        // The old highest sequence is now difference positions behind.
        state.receivedPacketBits |= uint64_t{1} << (difference - 1);
      }

      state.highestPacketSequence = packetSequence;
      state.ackPending = true;
      return true;
    }

    const uint64_t difference = state.highestPacketSequence - packetSequence;

    if (difference == 0)
      return false;

    if (difference > 64)
      return false;

    const uint64_t bit = uint64_t{1} << (difference - 1);

    if ((state.receivedPacketBits & bit) != 0)
      return false;

    state.receivedPacketBits |= bit;
    state.ackPending = true;
    return true;
  }

  void ProcessMessage(const AtlasNetNodeID& source, ReceiveState& state,
                      uint64_t messageSequence,
                      std::shared_ptr<const std::vector<std::byte>> storage,
                      size_t offset, size_t size)
  {
    switch (GetOptions().ordering)
    {
    case OrderingMode::Unordered:
      readyMessages_.push_back({
          .source = source,
          .sequence = messageSequence,
          .storage = std::move(storage),
          .offset = offset,
          .size = size,
      });
      break;

    case OrderingMode::Sequenced:
      if (messageSequence <= state.highestSequencedMessage)
        return;

      state.highestSequencedMessage = messageSequence;

      readyMessages_.push_back({
          .source = source,
          .sequence = messageSequence,
          .storage = std::move(storage),
          .offset = offset,
          .size = size,
      });
      break;

    case OrderingMode::Ordered:
      if (messageSequence < state.nextOrderedSequence)
      {
        // Duplicate or very late message.
        return;
      }

      if (messageSequence == state.nextOrderedSequence)
      {
        readyMessages_.push_back({
            .source = source,
            .sequence = messageSequence,
            .storage = std::move(storage),
            .offset = offset,
            .size = size,
        });

        ++state.nextOrderedSequence;
        DeliverOrderedMessages(source, state);
        return;
      }

      // A previous message is missing.
      state.reorderBuffer.try_emplace(messageSequence,
                                      BufferedMessage{
                                          .sequence = messageSequence,
                                          .storage = std::move(storage),
                                          .offset = offset,
                                          .size = size,
                                      });

      break;
    }
  }

  void DeliverOrderedMessages(const AtlasNetNodeID& source, ReceiveState& state)
  {
    while (true)
    {
      auto iterator = state.reorderBuffer.find(state.nextOrderedSequence);

      if (iterator == state.reorderBuffer.end())
        break;

      BufferedMessage buffered = std::move(iterator->second);

      state.reorderBuffer.erase(iterator);

      readyMessages_.push_back({
          .source = source,
          .sequence = buffered.sequence,
          .storage = std::move(buffered.storage),
          .offset = buffered.offset,
          .size = buffered.size,
      });

      ++state.nextOrderedSequence;
    }
  }

  size_t CopyReadyMessages(std::span<ClusterMessage> output)
  {
    size_t count = 0;

    while (count < output.size() && !readyMessages_.empty())
    {
      ReadyMessage ready = std::move(readyMessages_.front());

      readyMessages_.pop_front();

      output[count++] =
          ClusterMessage(ready.source, GetOptions().id, ready.sequence,
                         std::move(ready.storage), ready.offset, ready.size);
    }

    return count;
  }

  std::vector<std::byte> BuildPacket(PeerState& peer,
                                     std::span<const QueuedMessage> messages,
                                     uint64_t packetSequence, bool ackOnly)
  {
    ChannelV1::PacketHeader header;
    header.channel = GetOptions().id;
    header.packetSequence = packetSequence;
    header.ackSequence = peer.receive.highestPacketSequence;
    header.ackBits = peer.receive.receivedPacketBits;
    header.messageCount = static_cast<uint16_t>(messages.size());

    if (GetOptions().delivery == DeliveryMode::Reliable)
      header.flags = header.flags | ChannelV1::PacketFlags::Reliable;

    if (ackOnly)
      header.flags = header.flags | ChannelV1::PacketFlags::AckOnly;

    size_t payloadBytes = 0;

    for (const QueuedMessage& message : messages)
    {
      payloadBytes +=
          ChannelV1::MessageHeader::NetSize() + message.payload.size();
    }

    header.payloadBytes = static_cast<uint32_t>(payloadBytes);
    NetBinaryWriter writer;

    header.serialize(writer);

    for (const QueuedMessage& message : messages)
    {
      ChannelV1::MessageHeader messageHeader;
      messageHeader.messageSequence = message.sequence;
      messageHeader.payloadBytes =
          static_cast<uint32_t>(message.payload.size());
      messageHeader.serialize(writer);
      writer->adapter().writeBuffer<sizeof(uint8_t)>(
          reinterpret_cast<const uint8_t*>(message.payload.data()),
          message.payload.size());
    }

    peer.receive.ackPending = false;

    return writer.Release();
  }

  void SendPacket(const AtlasNetNodeID& destination, PeerState& peer,
                  std::vector<std::byte> bytes, uint64_t packetSequence)
  {
    GetOptions().transport->SendMessage(destination, bytes);

    if (GetOptions().delivery != DeliveryMode::Reliable)
      return;

    PendingPacket pending;
    pending.packetSequence = packetSequence;
    pending.bytes = std::move(bytes);
    pending.lastSent = Clock::now();
    pending.attempts = 1;

    peer.send.pendingPackets.emplace(packetSequence, std::move(pending));
  }
};
} // namespace AtlasNet::Network::Cluster