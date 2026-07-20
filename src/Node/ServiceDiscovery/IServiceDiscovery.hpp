#pragma once

#include "AtlasNet/Core/Core.hpp"
#include "AtlasNet/Core/Network/Address/MacAddress.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
namespace AtlasNet::Service
{
struct ServiceData
{
  std::string hostID;
  Network::SocketAddress internalAddress;
  Network::MACAddress macAddress;
};
struct ServiceEntry
{
  AtlasNetNodeID nodeID;
  ServiceData serviceData;
};
class IServiceLease
{
public:
  virtual ~IServiceLease() = default;
  virtual void Renew() = 0;
  virtual void Release() = 0;
};
class IServiceDiscovery
{
public:
  virtual ~IServiceDiscovery() = default;
  virtual std::unique_ptr<IServiceLease>
  RegisterService(const ServiceData& data) = 0;

  void SetOnServiceRegisteredCallback(
      std::function<void(const ServiceEntry&)> callback)
  {
    onServiceRegistered = callback;
  }
  void SetOnServiceUnregisteredCallback(
      std::function<void(const ServiceEntry&)> callback)
  {
    onServiceUnregistered = callback;
  }

private:
 void _onServiceRegistered(const ServiceEntry& entry)
  {
    if (onServiceRegistered)
    {
      onServiceRegistered(entry);
    }
  }
  void _onServiceUnregistered(const ServiceEntry& entry)
  {
    if (onServiceUnregistered)
    {
      onServiceUnregistered(entry);
    }
  }
  std::function<void(const ServiceEntry&)> onServiceRegistered;
  std::function<void(const ServiceEntry&)> onServiceUnregistered;

  
};
}; // namespace AtlasNet::Service