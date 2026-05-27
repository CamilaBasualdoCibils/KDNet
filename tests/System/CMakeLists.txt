add_executable(SystemTests ${CMAKE_CURRENT_SOURCE_DIR}/System.cpp)
target_compile_definitions(SystemTests
    PRIVATE
    TEST_COMPOSE_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(SystemTests
    AtlasNetTestCrashUtils
    gtest
)

add_test(
    NAME AtlasNetDevDockerBuildRuntime
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target AtlasNetDevDockerBuild --config $<CONFIG>
)
set_tests_properties(AtlasNetDevDockerBuildRuntime PROPERTIES
    FIXTURES_SETUP AtlasNetDevDockerImage
    LABELS "System"
)

gtest_discover_tests(
    SystemTests
    PROPERTIES
        LABELS "System"
        FIXTURES_REQUIRED "AtlasNetDevDockerImage"
)