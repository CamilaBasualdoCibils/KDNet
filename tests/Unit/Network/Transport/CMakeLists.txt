
add_executable(AtlasNet_tests_Transport ${CMAKE_CURRENT_SOURCE_DIR}/Transport.cpp)

target_link_libraries(AtlasNet_tests_Transport
AtlasNetCore
AtlasNetTestCrashUtils
gtest
)

gtest_discover_tests(AtlasNet_tests_Transport)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_Transport_TESTS} PROPERTIES LABELS "Transport")