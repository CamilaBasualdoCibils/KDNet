#pragma once
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/SocketAddress.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/database/redis/RedisConn.hpp"
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
#include <type_traits>
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
                                       const SocketAddress& address);
  template <typename OutputIt> uint64_t GetAllNodeIDs(OutputIt out) const
  {
    auto start = out;
    for_each_key_hashtable(_redisConn, NodeIDLeaseTable,
                           [&](const auto& rawID)
                           {
                             AtlasNetNodeID nodeID =
                                 AtlasNetNodeID::from_string(rawID);
                             *out++ = nodeID;
                           });
    return std::distance(start, out); // Return the number of node IDs written
  }
  template <std::output_iterator<AtlasNetShardID> OutputIt>
  uint64_t GetAllShardIDs(OutputIt out) const
  {
    auto start = out;
    for_each_key_hashtable(_redisConn, ShardIDLeaseTable,
                           [&](const auto& rawID)
                           {
                             AtlasNetShardID shardID =
                                 AtlasNetShardID::from_string(rawID);
                             *out++ = shardID;
                           });
    return std::distance(start, out); // Return the number of shard IDs written
  }

private:
  std::optional<AtlasNetNodeID> ClaimNodeID(const SocketAddress& address);
  std::optional<AtlasNetShardID> ClaimShardID(AtlasNetNodeID nodeID);
  std::optional<AtlasNetGatewayID> ClaimGatewayID(AtlasNetNodeID nodeID);
  std::optional<AtlasNetControllerID> ClaimControllerID(AtlasNetNodeID nodeID);

  template <typename Func>
  static void for_each_keyval_hashtable(Database::RedisConn* r,
                                        std::string_view TableName, Func f)
    requires std::is_invocable_v<Func, std::string_view, std::string_view>
  {
    boost::container::small_vector<std::string, 64> RawshardIDs;

    r->HashMap().GetSet().HGetAll(TableName, std::back_inserter(RawshardIDs));

    for (uint64_t i = 0; i < RawshardIDs.size() / 2; ++i)
    {
      const auto& rawShardID =
          RawshardIDs[i * 2]; // Get the key part, skipping the value
      const auto& rawNodeID =
          RawshardIDs[i * 2 +
                      1]; // Get the value part, corresponding to the key
      f(rawShardID, rawNodeID);
    }
  }

  template <typename Func>
  static void for_each_key_hashtable(Database::RedisConn* r,
                                     std::string_view TableName, Func f)
    requires std::is_invocable_v<Func, std::string_view>
  {
    boost::container::small_vector<std::string, 64> RawKeys;

    r->HashMap().GetSet().HGetAll(TableName, std::back_inserter(RawKeys));

    for (uint64_t i = 0; i < RawKeys.size(); ++i)
    {
      const auto& rawKey = RawKeys[i]; // Get the key part, skipping the value
      f(rawKey);
    }
  }

  const static inline std::string NodeRegistryNamespace =
      Env::DatabaseNamespace + "NodeRegistry:";
  const static inline std::string NodeIDLeaseTable = NodeRegistryNamespace +
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