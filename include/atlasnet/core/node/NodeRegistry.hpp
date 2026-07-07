#pragma once
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/database/redis/utils/RedisUtils.hpp"
#include "atlasnet/core/node/NodeTypes.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "enviroment/Enviroment.hpp"
#include "spdlog/logger.h"
#include "sw/redis++/redis.h"
#include <cassert>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <variant>
namespace AtlasNet
{

class NodeRegistry
{
  constexpr static int ID_LEASE_TTL_SECONDS = 30,
                       ID_LEASE_RENEW_INTERVAL_SECONDS = 10;
  class IDLease
  {
  public:
    IDLease(NodeRegistry& registry) : _registry(registry) {}
    void Init()
    {
      TTLThread = std::jthread(
          [this]()
          {
            while (true)
            {
              std::this_thread::sleep_for(
                  std::chrono::seconds(ID_LEASE_RENEW_INTERVAL_SECONDS));

              RenewIDLease();
            }
          });
    }

  protected:
    virtual void RenewIDLease() = 0;
    NodeRegistry& _registry;
    std::jthread TTLThread;
  };
  class IDLeaseHashMap : public IDLease
  {
  public:
    IDLeaseHashMap(NodeRegistry& registry, const std::string_view& hashName,
                   const std::string_view& ID)
        : IDLease(registry), _Hashname(hashName), _ID(ID)
    {
    }

  private:
    void RenewIDLease() override
    {
      _registry._redisConn->HashMap().TTL().HExpire(_Hashname, _ID,
                                                    ID_LEASE_TTL_SECONDS);
    }
    const std::string _Hashname;
    const std::string _ID;
  };

public:
  struct Config
  {
    Database::RedisConn* redisConn;
  };
  NodeRegistry(const Config& config)
  {
    _redisConn = config.redisConn;
    assert(_redisConn != nullptr);
  }

  /*
    struct ServiceInfo
    {

      HostAddress address;
      AtlasNetNodeType containerType;

      void Serialize(ByteWriter& archive) const
      {
        archive(id);
        archive(address);
        archive(containerType);
        // archive(overlayAddress);
      }

      void Deserialize(ByteReader& archive)
      {
        archive(id);
        archive(address);
        archive(containerType);
        // archive(overlayAddress);

      }

      void to_json(_Json& j) const
      {
        j = _Json{
            {"id", id.to_string()},
            {"address", address.to_string()},
            {"containerType",
             boost::describe::enum_to_string(containerType, "UNKNOWN")},
            //{"overlayAddress", overlayAddress.to_string()

        };
      }
    };
    void RegisterService(ServiceInfo info)
    {

      ByteWriter bwKey;
      bwKey(info.id);
      ByteWriter bw;
      info.Serialize(bw);
      _redisConn->HashMap().GetSet().HSet(ContainerID2ServiceInfoKey,
                                          bwKey.as_string_view(),
                                          bw.as_string_view());

      _redisConn->Set().Modify().SAdd(
          GetContainerType2ContainerIDsSetKey(info.containerType),
          bwKey.as_string_view());
      if (Env::DebugMode)
      {
        _Json debugJson;
        info.to_json(debugJson);
        _redisConn->HashMap().GetSet().HSet(ContainerID2ServiceInfoKey +
    "_debug", std::to_string(info.id), debugJson.dump(4));
        _redisConn->Set().Modify().SAdd(
            GetContainerType2ContainerIDsSetKey(info.containerType) + "_debug",
            std::to_string(info.id));
      }
    }

    void GetServicesOfType(AtlasNetNodeType type,
                           std::vector<ServiceInfo>& outServices)
    {
      std::unordered_set<AtlasNetNodeID> containerIDs;
      GetContainerIDsOfType(type, containerIDs);
      for (const auto& containerID : containerIDs)
      {
        ByteWriter bwKey;
        bwKey(containerID);

        if (std::optional<std::string> serviceInfoStr =
                _redisConn->HashMap().GetSet().HGet(ContainerID2ServiceInfoKey,
                                                    bwKey.as_string_view());
            serviceInfoStr.has_value())
        {
          ByteReader br(*serviceInfoStr);
          ServiceInfo info;
          info.Deserialize(br);
          outServices.push_back(std::move(info));
        }
      }
    }

    void GetContainerIDsOfType(AtlasNetNodeType type,
                               std::unordered_set<AtlasNetNodeID>& outNodeIDs)
    {
      std::unordered_set<std::string> containerIDStrs;
      _redisConn->Set().Query().SMembers(
          GetContainerType2ContainerIDsSetKey(type),
          std::inserter(containerIDStrs, containerIDStrs.end()));
      for (const auto& containerIDStr : containerIDStrs)
      {
        ByteReader br(containerIDStr);
        AtlasNetNodeID nodeID;
        br(nodeID);
        outNodeIDs.insert(nodeID);
      }
    }

    std::optional<ServiceInfo> GetServiceInfo(const AtlasNetNodeID& id)
    {
      ByteWriter bwKey;
      bwKey(id);
      if (std::optional<std::string> serviceInfoStr =
              _redisConn->HashMap().GetSet().HGet(ContainerID2ServiceInfoKey,
                                                  bwKey.as_string_view());
          serviceInfoStr.has_value())
      {
        ByteReader br(*serviceInfoStr);
        ServiceInfo info;
        info.Deserialize(br);
        return info;
      }
      return std::nullopt;
    } */

  std::optional<NodeInfo> RegisterNode(const AtlasNetNodeType type,
                                       const SocketAddress& address)
  {
    NodeInfo info;
    info.containerType = type;
    info.address = address;
    std::optional<AtlasNetNodeID> nodeID = ClaimNodeID(address);
    if (!nodeID.has_value())
    {
      logger->error("Failed to claim Node ID for address {}",
                    address.to_string());
      return std::nullopt;
    }
    info.id = *nodeID;

    if (type == AtlasNetNodeType::Controller)
    {
      ControllerNodeInfo controllerInfo;
      const std::optional<AtlasNetControllerID> controllerID =
          ClaimControllerID(info.id);
      if (!controllerID.has_value())
      {
        logger->error("Failed to claim Controller ID for node ID {}",
                      info.id.value);
        return std::nullopt;
      }
      controllerInfo.id = *controllerID;
      info.specificInfo = controllerInfo;
      // Handle Controller node registration
    }
    else if (type == AtlasNetNodeType::Shard)
    {
      ShardNodeInfo shardInfo;
      const std::optional<AtlasNetShardID> shardID = ClaimShardID(info.id);
      if (!shardID.has_value())
      {
        logger->error("Failed to claim Shard ID for node ID {}", info.id.value);
        return std::nullopt;
      }
      shardInfo.id = *shardID;
      info.specificInfo = shardInfo;
      // Handle Shard node registration
    }
    else if (type == AtlasNetNodeType::Gateway)
    {
      GatewayNodeInfo gatewayInfo;
      const std::optional<AtlasNetGatewayID> gatewayID =
          ClaimGatewayID(info.id);
      if (!gatewayID.has_value())
      {
        logger->error("Failed to claim Gateway ID for node ID {}",
                      info.id.value);
        return std::nullopt;
      }
      gatewayInfo.id = *gatewayID;
      info.specificInfo = gatewayInfo;
      // Handle Gateway node registration
    }
    else if (type == AtlasNetNodeType::WebBackend)
    {
      info.specificInfo = CartographBackendInfo{};
      // Handle CartographBackend node registration
    }
    else
    {
      throw std::runtime_error("Unknown node type");
    }
    return info;
  }
  /* template <typename OutputIt> uint64_t GetAllNodeIDs(OutputIt out) const
  {
    boost::container::small_vector<boost::container::small_vector<char, 32>, 64>
        RawnodeIDs;

    _redisConn->HashMap().GetSet().HGetAll(NodeIDReserveTable,
                                           std::back_inserter(RawnodeIDs));

    //HGETALL returns key,value,key,value,...
    for (uint64_t i = 0; i < RawnodeIDs.size() / 2; ++i)
    {
      const auto& rawID =
          RawnodeIDs[i * 2]; // Get the key part, skipping the value
      const auto& rawAddress =
          RawnodeIDs[i * 2 + 1]; // Get the value part, corresponding to the key
      AtlasNetNodeID nodeID;
      std::from_chars(rawID.data(), rawID.data() + rawID.size(), nodeID.value);
      SocketAddress address(
          std::string_view(rawAddress.data(), rawAddress.size()));

      *out++ = std::make_pair(nodeID, address);
    }

    return RawnodeIDs.size();
  }
  template <typename OutputIt> uint64_t GetAllShards(OutputIt out) const
  {
    boost::container::small_vector<boost::container::small_vector<char, 32>, 64>
        RawshardIDs;

    _redisConn->HashMap().GetSet().HGetAll(ShardIDLeaseTable,
                                           std::back_inserter(RawshardIDs));

    for (uint64_t i = 0; i < RawshardIDs.size() / 2; ++i)
    {
      const auto& rawShardID =
          RawshardIDs[i * 2]; // Get the key part, skipping the value
      const auto& rawNodeID =
          RawshardIDs[i * 2 +
                      1]; // Get the value part, corresponding to the key
      AtlasNetShardID shardID;
      std::from_chars(rawShardID.data(), rawShardID.data() + rawShardID.size(),
                      shardID.value);
      AtlasNetNodeID nodeID;
      std::from_chars(rawNodeID.data(), rawNodeID.data() + rawNodeID.size(),
                      nodeID.value);

      *out++ = std::make_pair(shardID, nodeID);
    }
    return RawshardIDs.size();
  } */

  std::optional<AtlasNetNodeID> ClaimNodeID(const SocketAddress& address)
  {
    if (NodeIdLease.has_value())
    {
      throw std::runtime_error("Node ID lease already exists");
    }

    logger->info("Claiming Node ID for address {} at table {} for max ID {}",
                 address.to_string(), NodeIDReserveTable,
                 std::numeric_limits<AtlasNetNodeID::underlying_type_t>::max());
    std::optional<uint64_t> result = ClaimIDHashTable(
        _redisConn, NodeIDReserveTable, address.to_string(),
        std::numeric_limits<AtlasNetNodeID::underlying_type_t>::max());
    if (!result.has_value())
    {
      return std::nullopt;
    }
    NodeIdLease.emplace(*this, NodeIDReserveTable,
                        std::to_string(result.value()));
    NodeIdLease->Init();
    return static_cast<AtlasNetNodeID>(*result);
  };
  std::optional<AtlasNetShardID> ClaimShardID(AtlasNetNodeID nodeID)
  {
    if (ShardIdLease.has_value())
    {
      throw std::runtime_error("Shard ID lease already exists");
    }

    logger->info(
        "Claiming Shard ID for node ID {} at table {} for max ID {}",
        nodeID.value, ShardIDLeaseTable,
        std::numeric_limits<AtlasNetShardID::underlying_type_t>::max());
    std::optional<uint64_t> result = ClaimIDHashTable(
        _redisConn, ShardIDLeaseTable, std::to_string(nodeID.value),
        std::numeric_limits<AtlasNetShardID::underlying_type_t>::max());
    if (!result.has_value())
    {
      return std::nullopt;
    }
    ShardIdLease.emplace(*this, ShardIDLeaseTable,
                         std::to_string(result.value()));
    ShardIdLease->Init();
    return static_cast<AtlasNetShardID>(*result);
  }

  std::optional<AtlasNetGatewayID> ClaimGatewayID(AtlasNetNodeID nodeID)
  {
    if (GatewayIdLease.has_value())
    {
      throw std::runtime_error("Gateway ID lease already exists");
    }

    logger->info(
        "Claiming Gateway ID for node ID {} at table {} for max ID {}",
        nodeID.value, GatewayIDLeaseTable,
        std::numeric_limits<AtlasNetGatewayID::underlying_type_t>::max());
    std::optional<uint64_t> result = ClaimIDHashTable(
        _redisConn, GatewayIDLeaseTable, std::to_string(nodeID.value),
        std::numeric_limits<AtlasNetGatewayID::underlying_type_t>::max());
    if (!result.has_value())
    {
      return std::nullopt;
    }
    GatewayIdLease.emplace(*this, GatewayIDLeaseTable,
                           std::to_string(result.value()));
    GatewayIdLease->Init();
    return static_cast<AtlasNetGatewayID>(*result);
  }
  std::optional<AtlasNetControllerID> ClaimControllerID(AtlasNetNodeID nodeID)
  {
    if (ControllerIdLease.has_value())
    {
      throw std::runtime_error("Controller ID lease already exists");
    }

    logger->info(
        "Claiming Controller ID for node ID {} at table {} for max ID {}",
        nodeID.value, ControllerIDLeaseTable,
        std::numeric_limits<AtlasNetControllerID::underlying_type_t>::max());
    std::optional<uint64_t> result = ClaimIDHashTable(
        _redisConn, ControllerIDLeaseTable, std::to_string(nodeID.value),
        std::numeric_limits<AtlasNetControllerID::underlying_type_t>::max());
    if (!result.has_value())
    {
      return std::nullopt;
    }
    ControllerIdLease.emplace(*this, ControllerIDLeaseTable,
                              std::to_string(result.value()));
    ControllerIdLease->Init();
    return static_cast<AtlasNetControllerID>(*result);
  }
  const static inline std::string NodeRegistryNamespace =
      Env::DatabaseNamespace + "NodeRegistry:";
  const static inline std::string NodeIDReserveTable = NodeRegistryNamespace +
                                                       "NodeIDLeaseTable",
                                  ShardIDLeaseTable = NodeRegistryNamespace +
                                                      "ShardIDLeaseTable",
                                  GatewayIDLeaseTable = NodeRegistryNamespace +
                                                        "GatewayIDLeaseTable",
                                  ControllerIDLeaseTable =
                                      NodeRegistryNamespace +
                                      "ControllerIDLeaseTable";
  /* std::string GetContainerType2ContainerIDsSetKey(AtlasNetNodeType type)
  {
    return ServiceInfoKeyPrefix +
           std::format(ContainerType2ContainerIDsSetKeyPostFix,
                       boost::describe::enum_to_string(type, "UNKNOWN"));
  } */
  /*  const std::string ServiceInfoKeyPrefix =
       Env::DatabaseNamespace + "ServiceRegistry:";
   const std::string ContainerID2ServiceInfoKey =
       ServiceInfoKeyPrefix + "ContainerID->ServiceInfo";
   constexpr static inline const char* ContainerType2ContainerIDsSetKeyPostFix =
       "{}->ContainerIDs"; */

  Database::RedisConn* _redisConn;
  std::optional<IDLeaseHashMap> NodeIdLease, ShardIdLease, GatewayIdLease,
      ControllerIdLease;
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("NodeRegistry");
};

} // namespace AtlasNet