




add_executable(AtlasNet_tests_Entity ${CMAKE_CURRENT_SOURCE_DIR}/EntityTests.cpp)
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_Entity
AtlasNetCore
gtest
)

gtest_discover_tests(AtlasNet_tests_Entity)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_Entity_TESTS} PROPERTIES LABELS "Entity")
