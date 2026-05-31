cmake_minimum_required(VERSION 3.10)

project(AtlasNet)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 ")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 ")

set (CMAKE_POLICY_VERSION_MINIMUM 3.5)
include(cmake/Deps.cmake)

set(ATLASNET_EXAMPLES ON CACHE BOOL "Build AtlasNet examples")
set(ATLASNET_TESTS ON CACHE BOOL "Build AtlasNet tests")
set(ATLASNET_DEV ON CACHE BOOL "Build AtlasNet dev")
set(ATLASNET_CARTOGRAPH ON CACHE BOOL "Build AtlasNet web interface")

add_custom_target(AtlasNetDocs
    COMMAND doxygen docs/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating documentation with Doxygen"
    VERBATIM
)
if (ATLASNET_EXAMPLES)
    add_subdirectory(examples)
endif()

if (ATLASNET_TESTS)
enable_testing()
    add_subdirectory(tests)
endif()

if (ATLASNET_DEV)
    add_subdirectory(dev)
endif()

# ========= ATLASNET CORE ========
file(GLOB_RECURSE _core_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
)

add_library(AtlasNetCore STATIC ${_core_sources})
# Optional, but common: local includes
target_include_directories(AtlasNetCore
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
    
)
target_link_libraries(AtlasNetCore
PUBLIC
GameNetworkingSockets::GameNetworkingSockets_s
OpenSSL::SSL
OpenSSL::Crypto
redis++::redis++_static
hiredis::hiredis
hiredis::hiredis_ssl
uv_a
Boost::headers
Boost::beast
Boost::bimap
Boost::describe
Boost::dynamic_bitset
Boost::flyweight
Boost::multi_array
Boost::multi_index
Boost::stacktrace_addr2line 
Boost::static_string
Boost::uuid
glm::glm
#Taskflow::Taskflow
EnTT
nlohmann_json::nlohmann_json
yaml-cpp::yaml-cpp
)


# ========= ATLASNET PROXY ========
file(GLOB_RECURSE _proxy_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/proxy/*.cpp"
)

add_executable(AtlasNetProxy ${_proxy_sources})
# Optional, but common: local includes
target_include_directories(AtlasNetProxy
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/proxy"
)
target_link_libraries(AtlasNetProxy AtlasNetCore)



# ========= ATLASNET CONTROLLER ========
file(GLOB_RECURSE _controller_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/controller/*.cpp"
)
add_executable(AtlasNetController ${_controller_sources})
# Optional, but common: local includes
target_include_directories(AtlasNetController
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/controller"
)
target_link_libraries(AtlasNetController AtlasNetCore)
target_compile_definitions(AtlasNetController PRIVATE ATLASNET_CONTROLLER_BUILD)

## ========= ATLASNET AGENT ========
#file(GLOB_RECURSE _agent_sources CONFIGURE_DEPENDS
#  "${CMAKE_CURRENT_SOURCE_DIR}/src/agent/*.cpp"
#)
#
#add_executable(AtlasNetAgent ${_agent_sources})
## Optional, but common: local includes
#target_include_directories(AtlasNetAgent
#      PRIVATE
#    "${CMAKE_CURRENT_SOURCE_DIR}/src/agent"
#)
#target_link_libraries(AtlasNetAgent AtlasNetCore)
#target_compile_definitions(AtlasNetAgent PRIVATE ATLASNET_AGENT_BUILD)
# ========= ATLASNET CARTOGRAPH ========
if (ATLASNET_CARTOGRAPH)
include(cmake/WebDeps.cmake)
    #directory(web)

file(GLOB_RECURSE _web_backend_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/web/backend/*.cpp"
)

add_executable(AtlasNetCartographBackend ${_web_backend_sources})
# Optional, but common: local includes
target_include_directories(AtlasNetCartographBackend
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/web/backend"
)
target_link_libraries(AtlasNetCartographBackend
AtlasNetCore
drogon
)
target_compile_definitions(AtlasNetCartographBackend PRIVATE ATLASNET_CARTOGRAPH_BACKEND_BUILD)
endif()

# ========= ATLASNET SHARD API ========
file(GLOB_RECURSE _API_shard_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/API/shard/*.cpp"
)
add_library(AtlasNetShardAPI STATIC ${_API_shard_sources})
add_library(AtlasNetShardAPI_s SHARED ${_API_shard_sources})
# Optional, but common: local includes
target_include_directories(AtlasNetShardAPI
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/API/shard"
      PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/API/shard"
)
target_include_directories(AtlasNetShardAPI_s
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/API/shard"
      PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/API/shard"
)
target_link_libraries(AtlasNetShardAPI AtlasNetCore)
target_link_libraries(AtlasNetShardAPI_s AtlasNetCore)

# ========= ATLASNET CLIENT API ========
file(GLOB_RECURSE _API_client_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/API/client/*.cpp"
)
add_library(AtlasNetClientAPI_s STATIC ${_API_client_sources})
add_library(AtlasNetClientAPI SHARED ${_API_client_sources})
# Optional, but common: local includes
target_include_directories(AtlasNetClientAPI
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/API/client"
      PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/API/client"
)
target_include_directories(AtlasNetClientAPI_s
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/API/client"
      PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/API/client"
)
target_link_libraries(AtlasNetClientAPI AtlasNetCore)
target_link_libraries(AtlasNetClientAPI_s AtlasNetCore)

# ========= ATLASNET DUMMY ========
file(GLOB_RECURSE _dummy_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/dummy/*.cpp"
)
add_executable(AtlasNetDummy ${_dummy_sources})
# Optional, but common: local includes
target_include_directories(AtlasNetDummy
      PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dummy"
)
target_link_libraries(AtlasNetDummy AtlasNetShardAPI_s)
target_compile_definitions(AtlasNetDummy PRIVATE ATLASNET_DUMMY_BUILD)

# ========= ATLASNET PHYSICS API ========
file(GLOB_RECURSE _physics_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/API/physics/*.cpp"
)
add_library(AtlasNetPhysicsAPI ${_physics_sources})
target_include_directories(AtlasNetPhysicsAPI
      PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/API/physics"
)
target_link_libraries(AtlasNetPhysicsAPI AtlasNetCore)

# ======== ATLASNET JOLT PHYSICS ========
file(GLOB_RECURSE _physics_jolt_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/physics/jolt/*.cpp"
)
add_executable(AtlasNetJoltPhysics ${_physics_jolt_sources})
target_compile_definitions(AtlasNetJoltPhysics PRIVATE ATLASNET_PHYSICS_JOLT_BUILD)
#target_link_libraries(AtlasNetJoltPhysics AtlasNetPhysicsAPI Jolt::Jolt)