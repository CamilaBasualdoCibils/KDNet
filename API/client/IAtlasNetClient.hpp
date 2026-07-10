#pragma once
#include "atlasnet/client/ClientRPC.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/core/CmdSig/command/Command.hpp"
#include "atlasnet/core/messages/MessageSystem.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "boost/describe/enum_to_string.hpp"
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
    taskSystem.emplace(TaskSystem::Config{});
    messageSystem.emplace(MessageSystem::Config{
        .taskSystem = &*taskSystem,
        .handshakeIdentity =
            HandshakeIdentity{.role = HandshakeRole::eClient,
                              .data = HandshakeClientRequestData{
                                  .payload = {'H', 'e', 'l', 'l', 'o'}}}});
    rpcSystem.emplace(RPCSystem::Config{.messageSystem = &*messageSystem});

    rpcSystem->Bind<ClientRPC::ClientConnectionCompleteNotification>(
        [this](const ClientConnectionCompleteData& data)
        { OnClientConnectionCompleteNotification(data); });
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
    TaskHandle<MessageConnectionResult> jobHandle = messageSystem->Connect(serverAddress);

    jobHandle->wait_for(std::chrono::seconds(5));
    if (jobHandle.GetTask().is_done())
    {
      logger->info("Successfully connected to server at {}.", serverAddress.to_string());
      logger->info("Waiting for connection complete notification...");

      std::unique_lock<std::mutex> lock(lastConnectionCompleteDataMutex);
      lastConnectionCompleteDataCV.wait_for(
          lock, std::chrono::seconds(5),
          [this]() { return lastConnectionCompleteData.has_value(); });

      if (!lastConnectionCompleteData.has_value())
      {
        if (error)
          *error = AtlasNetClientError::ConnectionTimedOut;
        if (errorMessage)
          *errorMessage = "Connection complete notification timed out.";
        return false;
      }
      logger->info("Received connection complete notification with state {}.",
                   boost::describe::enum_to_string(
                       lastConnectionCompleteData->result, "<INVALID>"));

      _serverAddress = SocketAddress(HostAddress(std::string(address)), port);
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

  void AtlasNetClient_DispatchCommand(
      const std::string_view& commandName, const std::string_view& commanddata,
      MessageSendMode sendMode = MessageSendMode::eReliableBatched)
  {
    ExternalCommandMessage message;
    message.envelope.commandName.assign(commandName.data(), commandName.size());
    message.envelope.payload.assign(commanddata.begin(), commanddata.end());
    MessageSendResult jobHandle = messageSystem->TrySendMessage(
        message, _serverAddress, sendMode);
  }

  template <typename CMD>
    requires(std::is_base_of_v<AtlasNet::ICommandSerializable, CMD>)
  void AtlasNetClient_DispatchCommand(
      const CMD& command,
      MessageSendMode sendMode = MessageSendMode::eReliableBatched)
  {
    ByteWriter writer;
    command.Serialize(writer);
    AtlasNetClient_DispatchCommand(command.GetName(), writer.as_string_view(),
                                   sendMode);
  }

private:
  void OnClientConnectionCompleteNotification(
      const ClientConnectionCompleteData& data)
  {
    lastConnectionCompleteData = data;
    lastConnectionCompleteDataCV.notify_all();
  }
  std::optional<ClientConnectionCompleteData> lastConnectionCompleteData;
  std::mutex lastConnectionCompleteDataMutex;
  std::condition_variable lastConnectionCompleteDataCV;

  SocketAddress _serverAddress;
  std::optional<TaskSystem> taskSystem;
  std::optional<MessageSystem> messageSystem;
  std::optional<RPCSystem> rpcSystem;

  std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("AtlasNetClient");
};
} // namespace AtlasNet