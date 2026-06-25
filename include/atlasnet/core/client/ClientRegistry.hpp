#pragma once
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/client/ClientDataEntry.hpp"
#include "atlasnet/core/container/Container.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/entity/Entity.hpp"
#include "atlasnet/core/events/GlobalEventSystem.hpp"
#include "atlasnet/core/serialize/ByteReader.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "enviroment/Enviroment.hpp"
namespace AtlasNet
{
class ClientRegistry
{
public:
  struct Config
  {
    GlobalEventSystem* _globalEventSystem;
    Database::RedisConn* __redisConn;
    IService* containerService;
  };

  ClientRegistry(const Config& config) : config_(config)
  {
    assert(config_._globalEventSystem &&
           "GlobalEventSystem pointer cannot be null");
    assert(config_.__redisConn && "RedisConn pointer cannot be null");
    assert(config_.containerService &&
           "Container service pointer cannot be null");
  };

  std::optional<ClientID> GetAddressClientID(const SocketAddress& address)
  {

    ByteWriter addressWriter;
    address.Serialize(addressWriter);
    std::optional<std::string> value =
        config_.__redisConn->HashMap().GetSet().HGet(
            AddressToClientIDHashKey, addressWriter.as_string_view());
    if (!value)
    {
      return std::nullopt;
    }
    ByteReader reader(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(value->data()), value->size()));
    ClientID clientID;
    reader.uuid(clientID);
    return clientID;
  }

  std::optional<SocketAddress> GetClientIDAddress(const ClientID& clientID)
  {
    ByteWriter clientIDWriter;
    clientIDWriter.uuid(clientID);
    std::optional<std::string> value =
        config_.__redisConn->HashMap().GetSet().HGet(
            ClientIDToAddressHashKey, clientIDWriter.as_string_view());
    if (!value)
    {
      return std::nullopt;
    }
    ByteReader reader(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(value->data()), value->size()));
    SocketAddress address;
    address.Deserialize(reader);
    return address;
  }
  void AssociateClientWithEntity(const ClientID& clientID,
                                 const EntityID& entityID)
  {
    ByteWriter clientIDWriter;
    clientIDWriter.uuid(clientID);
    ByteWriter entityIDWriter;
    entityIDWriter.uuid(entityID);

    assert(config_.__redisConn->HashMap().Exists().HExists(
               ClientID2EntityIDHashKey, clientIDWriter.as_string_view()) ==
               false &&
           "ClientID is already associated with an EntityID");

    assert(config_.__redisConn->HashMap().Exists().HExists(
               EntityID2ClientIDHashKey, entityIDWriter.as_string_view()) ==
               false &&
           "EntityID is already associated with a ClientID");
    config_.__redisConn->HashMap().GetSet().HSet(
        ClientID2EntityIDHashKey, clientIDWriter.as_string_view(),
        entityIDWriter.as_string_view());
    config_.__redisConn->HashMap().GetSet().HSet(
        EntityID2ClientIDHashKey, entityIDWriter.as_string_view(),
        clientIDWriter.as_string_view());

    if (Env::DebugMode)
    {
      config_.__redisConn->HashMap().GetSet().HSet(
          ClientID2EntityIDHashKey + "_debug", clientID.to_string(),
          entityID.to_string());
      config_.__redisConn->HashMap().GetSet().HSet(
          EntityID2ClientIDHashKey + "_debug", entityID.to_string(),
          clientID.to_string());
    }
  }
  struct LoginResult
  {

    ClientID clientID;
    Entity::Location SpawnLocation;
    std::vector<uint8_t>
        SpawnShardPayload; // This contains developer-defined data that will be
                           // given to the shard that spawns the client
  };
  [[nodiscard]] std::optional<LoginResult>
  LoginClient(const SocketAddress& address)
  {

    std::optional<ClientID> existingClientID = GetAddressClientID(address);
    if (existingClientID)
    {
      std::cerr << "Client with address " << address.to_string()
                << " is already logged in with ClientID: "
                << existingClientID->to_string() << std::endl;

      return std::nullopt; // Address is already logged in
    }

    ClientID newClientID = ClientID::Generate();
    ByteWriter addressWriter;
    LoginData entry;
    entry.address = address;
    entry.clientID = newClientID;
    entry.managingGateway = config_.containerService->GetID();
    entry.address.Serialize(addressWriter);
    ByteWriter clientIDWriter;
    clientIDWriter.uuid(newClientID);

    config_.__redisConn->HashMap().GetSet().HSet(
        AddressToClientIDHashKey, addressWriter.as_string_view(),
        clientIDWriter.as_string_view());
    config_.__redisConn->HashMap().GetSet().HSet(
        ClientIDToAddressHashKey, clientIDWriter.as_string_view(),
        addressWriter.as_string_view());

    ByteWriter entryWriter;
    entry.Serialize(entryWriter);
    config_.__redisConn->HashMap().GetSet().HSet(
        ClientDataHashKey, clientIDWriter.as_string_view(),
        entryWriter.as_string_view());

    if (Env::DebugMode)
    {
      _Json j;
      entry.to_json(j);
      config_.__redisConn->HashMap().GetSet().HSet(
          ClientDataHashKey + "_Debug", entry.clientID.to_string(), j.dump());

      config_.__redisConn->HashMap().GetSet().HSet(
          AddressToClientIDHashKey + "_Debug", address.to_string(),
          newClientID.to_string());
      config_.__redisConn->HashMap().GetSet().HSet(
          ClientIDToAddressHashKey + "_Debug", newClientID.to_string(),
          address.to_string());
    }
    return LoginResult{newClientID, Entity::Location{}};
  }

private:
  const Config config_;
  const std::string ClientRegistryNamespace =
      Env::DatabaseNamespace + "ClientRegistry{ATLASNET_CLIENT_REGISTRY}:";
  const std::string AddressToClientIDHashKey =
      ClientRegistryNamespace + "AddressToClientID";
  const std::string ClientIDToAddressHashKey =
      ClientRegistryNamespace + "ClientIDToAddress";
  const std::string ClientID2EntityIDHashKey =
      ClientRegistryNamespace + "ClientIDToEntityID";
  const std::string EntityID2ClientIDHashKey =
      ClientRegistryNamespace + "EntityIDToClientID";
  const std::string ClientDataHashKey = ClientRegistryNamespace + "ClientData";
};
} // namespace AtlasNet