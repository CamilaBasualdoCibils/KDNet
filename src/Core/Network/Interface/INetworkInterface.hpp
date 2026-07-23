#pragma once
#include "AtlasNet/Core/Network/Address/Address.hpp"
#include "AtlasNet/Core/Network/Address/MacAddress.hpp"
namespace AtlasNet::Network
{
class INetworkInterface
{
public:
    virtual ~INetworkInterface() = default;

    virtual Network::MACAddress GetMACAddress() const = 0;
    virtual Network::HostAddress GetHostAddress() const = 0;
    virtual std::string GetName() const = 0;
};
}