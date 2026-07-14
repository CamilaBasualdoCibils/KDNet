#include "AtlasNetGateway.hpp"
#include "atlasnet/client/ClientRPC.hpp"
#include "atlasnet/core/address/SocketAddress.hpp"
#include "atlasnet/shard/ShardRPC.hpp"
#include <iterator>

void AtlasNet::AtlasNetGateway::OnClientConnected(
    const ConnectionEstablishedEvent& event)
{
  GetLogger()->info("Gateway detected new client connection established: {}",
                    event.address.to_string());

  GetLogger()->info("Logging in new client at {}", event.address.to_string());
  const std::optional<ClientRegistry::LoginResult> entry =
      clientRegistry->LoginClient(event.address, GetGatewayID());
  if (!entry)
    return;

  GetLogger()->info("Declaring gateway relay for ClientID: {}",
                    entry->clientID.to_string());
  gatewayRelayService_->DeclareGatewayRelay(entry->clientID);

  // Eventually this will be implemented
  /* auto shardID_future =
      GetRPCSystem().Call<ControllerRPC::GetClosestShardToLocation>(
          GetControllerAddress(), entry->SpawnLocation);

  shardID_future.wait_for(std::chrono::seconds(5));
  if (shardID_future.valid())
  {
    const AtlasNetShardID shardID = shardID_future.get();
    logger->info("Received closest shard ID {} for client {}",
                 shardID.to_string(), entry->clientID.to_string());

  }
  else
  {
  logger->error("Failed to receive closest shard ID for client {} within
  timeout.", entry->clientID.to_string());

  } */
  GetLogger()->info("Fetching all shard IDs from the registry");
  boost::container::small_vector<AtlasNetShardID, 64> shardIDs;
  GetNodeRegistry().GetAllShardIDs(std::back_inserter(shardIDs));
  GetLogger()->info("Retrieved {} shard IDs from the registry",
                    shardIDs.size());
  if (shardIDs.empty())
  {
    GetLogger()->error("No shard IDs found in registry");
    throw std::runtime_error("No shard IDs found in registry");
  }

  GetLogger()->info("Using shard ID {}", shardIDs[0].to_string());
  ShardSpawnClientRequest request{
      .spawnTransform = entry->SpawnLocation.position,
      .clientID = entry->clientID,
      .gatewayRelayID = GetGatewayID(),
  };
  std::optional<SocketAddress> shardAddress;
  std::optional<AtlasNetShardID> resolvedShardID;
  for (const AtlasNetShardID& shardID : shardIDs)
  {
    GetLogger()->info("Attempting to resolve address for shard ID {}",
                      shardID.to_string());
    shardAddress = GetAddressResolver().ResolveShard(shardID);
    if (shardAddress)
    {
      GetLogger()->info("Resolved address for shard ID {}: {}",
                        shardIDs[0].to_string(), shardAddress->to_string());
      resolvedShardID = shardID;
      break;
    }

    GetLogger()->warn("Failed to resolve address for shard ID {}",
                      shardIDs[0].to_string());
  }

  auto spawnResult =
      GetRPCSystem().Call_R<ShardRPC_SpawnClient>(*shardAddress, request);

  std::future_status status = spawnResult.wait_for(std::chrono::seconds(5));
  std::optional<ShardSpawnClientResponse> spawnResponse;
  if (status == std::future_status::ready)
  {
    auto result = spawnResult.get();
    if (result.has_value())
    {
      spawnResponse = result.value();
    }
    else
    {
      GetLogger()->error(
          "Received error response for SpawnClient RPC call for client {}: {}",
          entry->clientID.to_string(),
          boost::describe::enum_to_string(result.error(),
                                         "<INVALID>"));
    }
  }

  if (spawnResponse)
  {

    GetLogger()->info(
        "Successfully spawned client entity with ID {} on shard {}",
        spawnResponse->entityID.to_string(), resolvedShardID->to_string());

    gatewayRelayService_->DeclareGatewayRelay(entry->clientID);
    clientRegistry->AssociateClientWithEntity(entry->clientID,
                                              spawnResponse->entityID);

    ClientConnectionCompleteData result;
    result.clientID = entry->clientID;
    result.entityID = spawnResponse->entityID;
    result.result = ClientConnectionResult::Success;

    GetRPCSystem().Call<ClientRPC_ClientConnectionCompleteNotification>(
        event.address, {result});
  }
  else
  {
    GetLogger()->error(
        "Failed to receive spawn result for client {} within timeout.",
        entry->clientID.to_string());
  }
}
