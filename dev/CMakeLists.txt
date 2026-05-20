

set(DEV_STAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.stage")

add_custom_target(AtlasNetDevDockerBuildStage
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEV_STAGE_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AtlasNetController>
        "${DEV_STAGE_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AtlasNetAgent>
        "${DEV_STAGE_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AtlasNetProxy>
        "${DEV_STAGE_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AtlasNetCartographBackend>
        "${DEV_STAGE_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AtlasNetDummy>
        "${DEV_STAGE_DIR}/"

    DEPENDS
        AtlasNetController
        AtlasNetAgent
        AtlasNetProxy
        AtlasNetCartographBackend
        AtlasNetDummy
    USES_TERMINAL
    VERBATIM
)

add_custom_target(AtlasNetDevDockerBuild
    COMMAND ./dev/BuildDevImages.sh
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS AtlasNetDevDockerBuildStage
    USES_TERMINAL
    VERBATIM
)
add_custom_target(AtlasNetDevDockerBuild_Valgrind
    COMMAND ./dev/BuildDevImages.sh "valgrind {EXE}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS AtlasNetDevDockerBuildStage
    USES_TERMINAL
    VERBATIM
)


if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(DOCKER_ARCH amd64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(DOCKER_ARCH arm64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7")
    set(DOCKER_ARCH arm/v7)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
    set(DOCKER_ARCH 386)
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

add_custom_target(AtlasNetProductionDockerBuild
    COMMAND ./dev/BuildProductionImages.sh ${DOCKER_ARCH}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL
    VERBATIM)