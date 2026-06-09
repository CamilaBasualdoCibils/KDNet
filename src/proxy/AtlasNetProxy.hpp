#pragma once

#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/events/MessagingEvents.hpp"
namespace AtlasNet
{
class AtlasNetProxy : public IService
{
public:
  AtlasNetProxy() : IService(ServiceType::Proxy) {}
  ~AtlasNetProxy() override = default;

  void OnInit() override {
    GetMessageSystem().OpenListenSocket(Env::ProxyListenPort);
    GetLocalEventSystem().On<ConnectionAcceptedInternallyPreHandshakeEvent>(
        [&](const ConnectionAcceptedInternallyPreHandshakeEvent& event)
        {
          std::cerr << "Proxy detected new incoming connection from "
                    << event.address.to_string() << std::endl;
        });
  }
  void OnShutdown() override {}
};
} // namespace AtlasNet