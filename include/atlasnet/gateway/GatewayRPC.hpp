#pragma once

#include "atlasnet/core/CmdSig/command/Command.hpp"
#include "atlasnet/core/RPC/RPCMacros.hpp"
namespace AtlasNet
{
ATLASNET_RPC(
    GatewayRPC,
    ATLASNET_RPC_METHOD(IngressCommand,
                        ATLASNET_RPC_SIG(CommandAck(
                            IngressCommandEnvelope))))
}