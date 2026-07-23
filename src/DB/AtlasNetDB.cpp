#include "AtlasNetDB.hpp"
#include "AtlasNet/Core/Network/Address/SocketAddress.hpp"
#include "AtlasNet/Core/Network/Transport/SteamNetSock/SteamNetSock.hpp"
#include "Network/Interface/LinuxNetworkInterface.hpp"
#include "valkeymodule.h"
#include <cstdlib>
#include <spdlog/spdlog.h>
int AtlasPingCommand(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
{
  ValkeyModule_ReplyWithSimpleString(ctx, "PONG");
  return VALKEYMODULE_OK;
}

extern "C"
{

  int ValkeyModule_OnLoad(ValkeyModuleCtx* ctx, ValkeyModuleString** argv,
                          int argc)
  {

    if (ValkeyModule_Init(ctx, "atlasdb", 1, VALKEYMODULE_APIVER_1) ==
        VALKEYMODULE_ERR)
    {
      return VALKEYMODULE_ERR;
    }
    ValkeyModule_Log(ctx, "notice", "AtlasNet Valkey Module loading...");

    if (ValkeyModule_CreateCommand(
            ctx, "atlas.ping",
            [](ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
            { return AtlasPingCommand(ctx, argv, argc); }, "readonly", 0, 0,
            0) == VALKEYMODULE_ERR)
    {
      return VALKEYMODULE_ERR;
    }

    return VALKEYMODULE_OK;
  }
}
void AtlasNet::AtlasNetDB::Initialize()
{
  logger->info("Initializing AtlasNet-DB");
  if (const auto socketTypeEnv = std::getenv("SOCKET_TYPE"))
  {
    const bool parsedSocketType =
        boost::describe::enum_from_string(socketTypeEnv, options.socket_type);
    if (!parsedSocketType)
    {
      logger->error("Invalid socket type {}", socketTypeEnv);
      throw std::runtime_error("Invalid socket type");
    }
  }
  else
  {
    logger->info("SOCKET_TYPE not set, using default: SteamNetSock");
    options.socket_type = Network::SocketType::SteamNetSock;
  }

  if (!network_interface)
  {
    network_interface =
        std::make_shared<Network::LinuxNetworkInterface>("eno1");
    logger->info("Using network interface: {}", network_interface->GetName());
  }
  if (!transport)
  {
    switch (options.socket_type)
    {
    case Network::SocketType::SteamNetSock:
      transport = std::make_unique<Network::SteamNetSockTransport>();
      break;
    case Network::SocketType::TCP:
      throw std::runtime_error("TCP transport not implemented yet");
      // transport = std::make_unique<Network::TCPTransport>();
      break;
    case Network::SocketType::WebSocket:
      throw std::runtime_error("WebSocket transport not implemented yet");
      // transport = std::make_unique<Network::WebSocketTransport>();
      break;
    case Network::SocketType::INVALID:
    default:
      throw std::runtime_error("Invalid socket type");
    }
  }
  listener = transport->Listen(Network::SocketAddress(
      network_interface->GetHostAddress(), options.network_port));
  if (!listener)
  {
    throw std::runtime_error("Failed to create listener");
  }
  listener->SetConnectionRequestCallback(
      [this](Network::ConnectionRequest& request, Network::IListener& listener)
      { HandleConnectionRequest(request, listener); });
}
void AtlasNet::AtlasNetDB::HandleConnectionRequest(
    Network::ConnectionRequest& request, Network::IListener& listener)
{
  logger->info("Received connection request from {}",
               request.RemoteAddress().to_string());
}
