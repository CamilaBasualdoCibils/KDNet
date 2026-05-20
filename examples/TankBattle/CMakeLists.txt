cmake_minimum_required(VERSION 3.10)

# Recursively collect sources under ./src
file(GLOB_RECURSE _core_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
)

add_library(TankBattleCore ${_core_sources})
# Optional, but common: local includes
target_include_directories(TankBattleCore
      PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
)


# Recursively collect sources under ./src
file(GLOB_RECURSE _shard_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/shard/*.cpp"
)

add_executable(TankBattleShard ${_shard_sources})
# Optional, but common: local includes
target_include_directories(TankBattleShard
      PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shard"
)

target_link_libraries(TankBattleShard TankBattleCore AtlasNetShardAPI_s)


add_custom_target(TankBattleStage
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/.stage"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:TankBattleShard>
        "${CMAKE_CURRENT_SOURCE_DIR}/.stage/"

    DEPENDS TankBattleShard
)
add_custom_target(TankBattleDockerDevBuild
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND docker build -t tankbattle-dev:latest .
DEPENDS TankBattleStage
)

add_custom_target(TankBattleDockerDeploy
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    # If the stack exists, kill it and wait 1 second
    COMMAND docker stack rm tankbattle || true
    COMMAND sleep 1
    COMMAND docker stack deploy -c TankBattleStack.yml tankbattle
DEPENDS TankBattleDockerDevBuild AtlasNetDevDockerBuild
)