#pragma once

#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/RPC/RPCConcepts.hpp"
#include "atlasnet/core/RPC/RPCSystem.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"

namespace AtlasNet
{
struct DeclareSelfAgentReadyRequest
{
  ContainerID agentID;
  HostAddress agentAddress;
  uint32_t cpuCount;

  void Serialize(ByteWriter& writer) const
  {
    writer(agentID);
    writer(agentAddress);
    writer(cpuCount);
  }
  void Deserialize(ByteReader& reader)
  {
    reader(agentID);
    reader(agentAddress);
    reader(cpuCount);
  }
};
ATLASNET_RPC(ControllerRPC, ATLASNET_RPC_METHOD(DeclareSelfAgentReady, void,
                                                DeclareSelfAgentReadyRequest););
} // namespace AtlasNet