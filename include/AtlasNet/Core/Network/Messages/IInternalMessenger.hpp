#pragma once
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Messages/MessageRecepient.hpp"
#include "AtlasNet/Core/Network/NetworkPacket.hpp"
namespace AtlasNet::Network::Messages
{
class IInternalMessenger
{
public:
  void Send(Network::PacketPayloadView data, const VRecepient& recepient)
  {
    std::visit([&](auto&& recepient) { ResolveAndSend(data, recepient); },
               recepient);
  }
  template <typename Message>
  void Send(const Message::PayloadType& payload, const VRecepient& recepient)
  {
    Network::PacketPayloadView data = Network::PacketPayloadView::FromMessage<Message>(payload);
    Send(data, recepient);
  }

private:
  void ResolveAndSend(Network::PacketPayloadView data,
                      const Recepient::NodeRecepient& recepient)
  {
    // Implementation for sending to a node
  }
  void ResolveAndSend(Network::PacketPayloadView data,
                      const Recepient::ShardOfEntityRecepient& recepient)
  {
    // Implementation for sending to a shard of an entity
  }
  void ResolveAndSend(Network::PacketPayloadView data,
                      const Recepient::ShardOfClientRecepient& recepient)
  {
    // Implementation for sending to a shard of a client
  }
  void ResolveAndSend(Network::PacketPayloadView data,
                      const Recepient::DatabaseRecepient& recepient)
  {
    // Implementation for sending to the database
  }

  virtual void __Send(Network::PacketPayloadView data,
                      const SocketAddress& address) = 0;
};
} // namespace AtlasNet::Network::Messages