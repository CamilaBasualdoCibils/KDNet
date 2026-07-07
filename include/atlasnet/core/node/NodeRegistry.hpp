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
  class INodeIDLease
  {
  public:
    INodeIDLease(NodeRegistry& registry) : _registry(registry)
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
  class NodeIDLeaseHashMap : public INodeIDLease
  {
  public:
    NodeIDLeaseHashMap(NodeRegistry& registry, const std::string_view& hashName,
                       const std::string_view& ID)
        : INodeIDLease(registry), _Hashname(hashName), _ID(ID)
    {
    }

  private:
    void RenewIDLease() override
    {
      std::string command[] = {"HEXPIRE", _Hashname,
                               std::to_string(ID_LEASE_TTL_SECONDS), "FIELDS",
                               _ID};
      std::optional<long long> result =
          _registry._redisConn->Command<long long>(std::begin(command),
                                                   std::end(command));
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
                                       const HostAddress& address)
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
      info.specificInfo = ControllerNodeInfo{};
      // Handle Controller node registration
    }
    else if (type == AtlasNetNodeType::Shard)
    {
      info.specificInfo = ShardNodeInfo{};
      // Handle Shard node registration
    }
    else if (type == AtlasNetNodeType::Gateway)
    {
      info.specificInfo = GatewayNodeInfo{};
      // Handle Gateway node registration
    }
    else
    {
      throw std::runtime_error("Unknown node type");
    }
    return info;
  }

private:
  std::optional<AtlasNetNodeID> ClaimNodeID(const HostAddress& address)
  {
    if (NodeIdLease.has_value())
    {
      throw std::runtime_error("Node ID lease already exists");
    }
    ByteWriter addressWriter;
    addressWriter(address);
    std::optional<uint64_t> result = ClaimIDHashTable(
        _redisConn, NodeIDReserveTable, addressWriter.as_string_view(),
        std::numeric_limits<AtlasNetNodeID>::max());
    if (!result.has_value())
    {
      return std::nullopt;
    }
    NodeIdLease.emplace(NodeIDLeaseHashMap(*this, NodeIDReserveTable,
                                           std::to_string(result.value())));
    return static_cast<AtlasNetNodeID>(*result);
  };

  constexpr static inline const char* NodeIDReserveTable = "NodeIDReserveTable";
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
  std::optional<NodeIDLeaseHashMap> NodeIdLease;
  std::shared_ptr<spdlog::logger> logger =
      spdlog::stdout_color_mt("NodeRegistry");
};

} // namespace AtlasNet