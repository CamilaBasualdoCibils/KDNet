#pragma once

#include "AtlasNet/Core/Network/Address/MacAddress.hpp"
#include "Network/Interface/INetworkInterface.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <linux/if.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace AtlasNet::Network
{
class LinuxNetworkInterface : public INetworkInterface
{
  const std::string interface;
  MACAddress macAddress;

public:
  Network::HostAddress GetHostAddress() const override
  {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
      throw std::runtime_error("Failed to create socket");
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
    {
      close(fd);
      throw std::runtime_error("Failed to get interface address");
    }

    close(fd);

    const auto* addr =
        reinterpret_cast<const sockaddr_in*>(&ifr.ifr_addr);

    char buffer[INET_ADDRSTRLEN]{};
    if (inet_ntop(AF_INET, &addr->sin_addr, buffer, sizeof(buffer)) == nullptr)
    {
      throw std::runtime_error("Failed to convert interface address");
    }

    return Network::HostAddress(buffer);
  }

  std::string GetName() const override
  {
    return interface;
  }

  explicit LinuxNetworkInterface(const std::string& interface)
      : interface(interface)
  {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
    {
      close(fd);
      throw std::runtime_error("Failed to get MAC address");
    }

    close(fd);

    std::array<uint8_t, 6> bytes;
    std::memcpy(bytes.data(), ifr.ifr_hwaddr.sa_data, 6);

    macAddress = MACAddress(bytes);
  }

  MACAddress GetMACAddress() const override
  {
    return macAddress;
  }
};
}; // namespace AtlasNet::Network