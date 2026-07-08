




add_executable(AtlasNet_tests_Spatial ${CMAKE_CURRENT_SOURCE_DIR}/SpatialTests.cpp)
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_Spatial
AtlasNetTestCrashUtils
gtest
)

gtest_discover_tests(AtlasNet_tests_Spatial)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_Spatial_TESTS} PROPERTIES LABELS "Spatial")
