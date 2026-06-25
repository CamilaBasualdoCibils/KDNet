
add_executable(AtlasNetMessagePerformanceTests ${CMAKE_CURRENT_SOURCE_DIR}/MessagesPerf.cpp)

target_link_libraries(AtlasNetMessagePerformanceTests
AtlasNetCore
AtlasNetTestCrashUtils
gtest
)

gtest_discover_tests(AtlasNetMessagePerformanceTests)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNetMessagePerformanceTests_TESTS} PROPERTIES LABELS "MessagesPerformance")