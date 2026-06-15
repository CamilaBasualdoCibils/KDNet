#pragma once
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/job/JobHandle.hpp"
#include "atlasnet/core/job/JobSystem.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include <cstdint>
#include <string>
#include <string_view>
namespace AtlasNet
{
class IAtlasNetClient
{
public:
  void AtlasNetClient_Init()
  {
    jobSystem.emplace(JobSystem::Config{});
    messageSystem.emplace(MessageSystem::Config{
        .jobSystem = &*jobSystem,
        .handshakeIdentity =
            HandshakeIdentity{.role = HandshakeRole::eClient,
                              .data = HandshakeClientRequestData{
                                  .payload = {'H', 'e', 'l', 'l', 'o'}}}});
    rpcSystem.emplace(RPCSystem::Config{.messageSystem = &*messageSystem});
  }

  enum class AtlasNetClientError
  {
    None,
    ConnectionTimedOut,
    UnknownError
  };
  bool AtlasNetClient_Connect(const std::string_view& address, uint16_t port,
                              AtlasNetClientError* error = nullptr,
                              std::string* errorMessage = nullptr)
  {
    SocketAddress serverAddress(HostAddress(std::string(address)), port);
    JobHandle jobHandle = messageSystem->Connect(serverAddress);

    jobHandle.wait(std::chrono::seconds(5));
    if (jobHandle.is_completed())
    {
      if (error)
        *error = AtlasNetClientError::None;
      if (errorMessage)
        *errorMessage = "";
      return true;
    }
    else
    {
      if (error)
        *error = AtlasNetClientError::ConnectionTimedOut;
      if (errorMessage)
        *errorMessage = "Connection attempt timed out.";
      return false;
    }
  }

private:
  std::optional<JobSystem> jobSystem;
  std::optional<MessageSystem> messageSystem;
  std::optional<RPCSystem> rpcSystem;
};
} // namespace AtlasNet