#pragma once
#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Intent/ClusterIntentChannel.hpp"
#include "AtlasNet/Core/Network/Intent/IntentRecepient.hpp"
#include "AtlasNet/Core/Network/RPC/RPC.hpp"
#include "RPC.hpp"
namespace AtlasNet::Network::RPC
{
struct ClusterRPCCaller
{
  Intent::VIntent intent;
  AtlasNetNodeID nodeID;
};
class ClusterRPC : public RPC<Intent::VIntent, ClusterRPCCaller>
{
public:
  struct Config
  {
    std::shared_ptr<Intent::ClusterIntentChannel> clusterIntentChannel;
  };
  ClusterRPC(const Config& c) : config_(c) {}

protected:
  void _ImplSendPacket(const Intent::VIntent& target,
                       std::span<const std::byte> packetData) override
  {
    config_.clusterIntentChannel->Send(target, packetData);
  }

  void _ImplPoll() override
  {
    config_.clusterIntentChannel->Tick();
    constexpr size_t receiveBufferSize = 32;
    std::array<Intent::IntentDatagram, receiveBufferSize> receiveBuffer;
    while (true)
    {
      size_t receiveNum =
          config_.clusterIntentChannel->TryReceive(receiveBuffer);
      for (size_t i = 0; i < receiveNum; ++i)
      {
        ClusterRPCCaller caller{receiveBuffer[i].Header().intent,
                                receiveBuffer[i].Header().source};
        HandleIncomingPacket(caller, receiveBuffer[i].Payload());
      }
      if (receiveNum < receiveBufferSize)
      {
        break;
      }
    }
  }

  std::string TargetToString(const Intent::VIntent& target) const override
  {
    return Intent::IntentToString(target);
  }

private:
  Config config_;
};
} // namespace AtlasNet::Network::RPC