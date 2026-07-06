




add_executable(SnowflakeTests ${CMAKE_CURRENT_SOURCE_DIR}/SnowflakeTests.cpp)
# Optional, but common: local includes

target_link_libraries(SnowflakeTests
AtlasNetTestCrashUtils
gtest
)

gtest_discover_tests(SnowflakeTests)

# Optional labels for filtering in ctest
set_tests_properties(${SnowflakeTests_TESTS} PROPERTIES LABELS "Snowflake")
