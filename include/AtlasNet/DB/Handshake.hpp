#pragma once

#include "AtlasNet/Core/Network/RPC/RPCMethod.hpp"
namespace AtlasNet::DB
{

using RPC_DB_Ping =
    Network::RPC::RPCMethod<"AtlasNet.DB.Ping", int,int>;

struct RegisterNodeRequest
{
    template <typename Archive>
    void serialize(Archive& ar)
    {

    }
};
struct RegisterNodeResponse
{
    template <typename Archive>
    void serialize(Archive& ar)
    {

    }
};
using RPC_DB_RegisterNode =
    Network::RPC::RPCMethod<"AtlasNet.DB.RegisterNode", RegisterNodeResponse,
                            RegisterNodeRequest>;
} // namespace AtlasNet::DB