#pragma once

#include "AtlasNet/Core/Network/Cluster/Channel/IClusterChannel.hpp"
#include <memory>
namespace AtlasNet::Network::Cluster
{
    class ChannelBus
    {

        public:

        struct ChannelOptions
        {
            
        };
        std::shared_ptr<IClusterChannel> MakeChannel(const ChannelOptions& options)
        {
            return std::make_shared<IClusterChannel>(options);
        }

        bool SendMessage(const AtlasNetNodeID& destination, std::span<const std::byte> payload)
        {
        };
    };
}