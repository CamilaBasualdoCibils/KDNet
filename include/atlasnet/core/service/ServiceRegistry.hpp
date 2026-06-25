#pragma once
#include "atlasnet/core/Address.hpp"
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/container/ContainerEnums.hpp"
#include "atlasnet/core/database/redis/Redis.hpp"
#include "atlasnet/core/serialize/ByteWriter.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "enviroment/Enviroment.hpp"
#include <cassert>
#include <iterator>
#include <optional>
#include <unordered_map>
namespace AtlasNet
{
class ServiceRegistry
{

public:
  struct Config
  {
    Database::RedisConn* redisConn;
  };
  ServiceRegistry(const Config& config)
  {
    _redisConn = config.redisConn;
    assert(_redisConn != nullptr);
  }

  struct ServiceInfo
  {
    ServiceID id;
    HostAddress address;
    ServiceType containerType;

    void Serialize(ByteWriter& archive) const
    {
      archive((UUID)id);
      archive(address);
      archive(containerType);
      // archive(overlayAddress);
    }

    void Deserialize(ByteReader& archive)
    {
      UUID id_uuid;
      archive(id_uuid);
      id = ServiceID(id_uuid);
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
    bwKey.uuid((UUID)info.id);
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
      _redisConn->HashMap().GetSet().HSet(ContainerID2ServiceInfoKey + "_debug",
                                          info.id.to_string(),
                                          debugJson.dump(4));
      _redisConn->Set().Modify().SAdd(
          GetContainerType2ContainerIDsSetKey(info.containerType) + "_debug",
          info.id.to_string());
    }
  }

  void GetServicesOfType(ServiceType type,
                         std::vector<ServiceInfo>& outServices)
  {
    std::unordered_set<ServiceID> containerIDs;
    GetContainerIDsOfType(type, containerIDs);
    for (const auto& containerID : containerIDs)
    {
      ByteWriter bwKey;
      bwKey.uuid((UUID)containerID);

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

  void GetContainerIDsOfType(ServiceType type,
                             std::unordered_set<ServiceID>& outContainerIDs)
  {
    std::unordered_set<std::string> containerIDStrs;
    _redisConn->Set().Query().SMembers(
        GetContainerType2ContainerIDsSetKey(type),
        std::inserter(containerIDStrs, containerIDStrs.end()));
    for (const auto& containerIDStr : containerIDStrs)
    {
      ByteReader br(containerIDStr);
      UUID containerID_uuid;
      br.uuid(containerID_uuid);
      outContainerIDs.insert(ServiceID(containerID_uuid));
    }
  }

  std::optional<ServiceInfo> GetServiceInfo(const ServiceID& id)
  {
    ByteWriter bwKey;
    bwKey.uuid((UUID)id);
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
  }
private:
  std::string GetContainerType2ContainerIDsSetKey(ServiceType type)
  {
    return ServiceInfoKeyPrefix +
           std::format(ContainerType2ContainerIDsSetKeyPostFix,
                       boost::describe::enum_to_string(type, "UNKNOWN"));
  }
  const std::string ServiceInfoKeyPrefix =
      Env::DatabaseNamespace + "ServiceRegistry:";
  const std::string ContainerID2ServiceInfoKey =
      ServiceInfoKeyPrefix + "ContainerID->ServiceInfo";
  constexpr static inline const char* ContainerType2ContainerIDsSetKeyPostFix =
      "{}->ContainerIDs";

  Database::RedisConn* _redisConn;
};

} // namespace AtlasNet