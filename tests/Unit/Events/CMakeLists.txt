




add_executable(AtlasNet_tests_LocalEvents ${CMAKE_CURRENT_SOURCE_DIR}/Local/LocalEventTests.cpp)
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_LocalEvents
AtlasNetCore
gtest
)

gtest_discover_tests(AtlasNet_tests_LocalEvents)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_LocalEvents_TESTS} PROPERTIES LABELS "Event")







add_executable(AtlasNet_tests_GlobalEvents ${CMAKE_CURRENT_SOURCE_DIR}/Global/GlobalEventTests.cpp)
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_GlobalEvents
AtlasNetCore
gtest
)

gtest_discover_tests(AtlasNet_tests_GlobalEvents)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_GlobalEvents_TESTS} PROPERTIES LABELS "Event")
