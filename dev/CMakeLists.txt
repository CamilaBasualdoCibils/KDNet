

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
        $<TARGET_FILE:AtlasNetWebBackend>
        "${DEV_STAGE_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:AtlasNetDummy>
        "${DEV_STAGE_DIR}/"

    DEPENDS
        AtlasNetController
        AtlasNetAgent
        AtlasNetProxy
        AtlasNetWebBackend
        AtlasNetDummy
)

add_custom_target(AtlasNetDevDockerBuild
    COMMAND ./dev/BuildDevImages.sh
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS AtlasNetDevDockerBuildStage
)
add_custom_target(AtlasNetDevDockerBuild_Valgrind
    COMMAND ./dev/BuildDevImages.sh "valgrind {EXE}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS AtlasNetDevDockerBuildStage
)