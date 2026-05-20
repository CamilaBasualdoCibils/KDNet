#pragma once
#include "atlasnet/core/Json.hpp"
#include "atlasnet/core/UUID.hpp"
#include "atlasnet/core/entity/collider/Collider.hpp"
#include "atlasnet/core/geometry/Vec.hpp"
#include "boost/container/small_vector.hpp"
#include "boost/describe/enum_from_string.hpp"
#include "boost/describe/enum_to_string.hpp"
#include "entt/entity/fwd.hpp"
#include <entt/entt.hpp>
#include <type_traits>
#include <variant>

namespace AtlasNet
{
namespace Entity
{

struct EntityIDTag
{
};
struct ClientIDTag
{
};
using EntityID = StrongUUID<EntityIDTag>;

#define ENTT_STANDARD_CPP
using ClientID = StrongUUID<ClientIDTag>;
using WorldID = uint32_t;
struct Transform
{
  vec3 position;
  void to_json(_Json& j) const
  {
    j = _Json{{"position",
               {{"x", position.x}, {"y", position.y}, {"z", position.z}}}};
  }
  void from_json(const _Json& j)
  {
    position.x = j.at("position").at("x").get<float>();
    position.y = j.at("position").at("y").get<float>();
    position.z = j.at("position").at("z").get<float>();
  }
};
struct Location
{
  WorldID worldId;
  Transform transform;

  void to_json(_Json& j) const
  {
    _Json transformJson;
    transform.to_json(transformJson);
    j = _Json{{"worldId", worldId}, {"transform", transformJson}};
  }

  void from_json(const _Json& j)
  {
    worldId = j.at("worldId").get<WorldID>();
    transform.from_json(j.at("transform"));
  }
};
namespace Components
{
struct EntityComponent
{
};
struct EntityInfo : public EntityComponent
{
  EntityID id;
  Location location;
  void to_json(_Json& j) const
  {

    _Json locationJson;
    location.to_json(locationJson);
    j = _Json{{"id", id.to_string()}, {"location", locationJson}};
  }
  void from_json(const _Json& j)
  {
    id = (EntityID)EntityID::from_string(j.at("id").get<std::string>());
    location.from_json(j.at("location"));
  }
};
struct ActorInfo : public EntityComponent
{
  int pad;
  // Data is not required because we dont store, we serialize on transform from
  // IAtlasNetShard boost::container::small_vector<uint8_t, 256> payload;
  void to_json(_Json& j) const
  {
    j = _Json{};
  }
  void from_json(const _Json& j) {}
};
struct ClientInfo : public EntityComponent
{
  ClientID id;
  void to_json(_Json& j) const
  {
    j = _Json{{"id", id.to_string()}};
  }
  void from_json(const _Json& j)
  {
    id = (ClientID)ClientID::from_string(j.at("id").get<std::string>());
  }
};
struct ColliderInfo : public EntityComponent
{
  std::variant<std::monostate, BoxCollider, SphereCollider> collider;

  void to_json(_Json& j) const
  {
    j = _Json::object();

    std::visit(
        [&j](const auto& col)
        {
          using T = std::decay_t<decltype(col)>;

          if constexpr (std::is_same_v<T, std::monostate>)
          {
            j["type"] = "NONE";
          }
          else
          {
            j["type"] =
                boost::describe::enum_to_string(col.get_type(), "UNKNOWN");
            col.to_json(j);
          }
        },
        collider);
  }

  void from_json(const _Json& j)
  {
    if (!j.contains("type"))
    {
      collider = std::monostate{};
      return;
    }

    const auto typeStr = j.at("type").get<std::string>();
    if (typeStr == "NONE")
    {
      collider = std::monostate{};
      return;
    }

    ColliderType type{};
    if (!boost::describe::enum_from_string(typeStr.c_str(), type))
    {
      collider = std::monostate{};
      return;
    }

    if (type == ColliderType::Box)
    {
      collider = BoxCollider(j);
    }
    else if (type == ColliderType::Sphere)
    {
      collider = SphereCollider(j);
    }
    else
    {
      collider = std::monostate{};
    }
  }
};

} // namespace Components

// using EntityTabl2 = entt::basic_registry<Entitye>;
using EntityTable = entt::registry;
struct Entity_Serializable
{
  std::optional<Components::EntityInfo> entityInfo;
  std::optional<Components::ActorInfo> actorInfo;
  std::optional<Components::ClientInfo> clientInfo;
  std::optional<Components::ColliderInfo> colliderInfo;

  void to_json(_Json& j) const
  {
    if (entityInfo)
      entityInfo->to_json(j["entityInfo"]);
    if (actorInfo)
      actorInfo->to_json(j["actorInfo"]);
    if (clientInfo)
      clientInfo->to_json(j["clientInfo"]);
    if (colliderInfo)
      colliderInfo->to_json(j["colliderInfo"]);
  }
  void from_json(const _Json& j)
  {
    if (j.contains("entityInfo"))
    {
      entityInfo.emplace();
      entityInfo->from_json(j.at("entityInfo"));
    }
    if (j.contains("actorInfo"))
    {
      actorInfo.emplace();
      actorInfo->from_json(j.at("actorInfo"));
    }
    if (j.contains("clientInfo"))
    {
      clientInfo.emplace();
      clientInfo->from_json(j.at("clientInfo"));
    }
    if (j.contains("colliderInfo"))
    {
      colliderInfo.emplace();
      colliderInfo->from_json(j.at("colliderInfo"));
    }
  };
};
} // namespace Entity
} // namespace AtlasNet
