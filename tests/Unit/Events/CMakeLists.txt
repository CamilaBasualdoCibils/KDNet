




add_executable(LocalEventTests ${CMAKE_CURRENT_SOURCE_DIR}/Local/LocalEventTests.cpp)
# Optional, but common: local includes

target_link_libraries(LocalEventTests
AtlasNetCore
gtest
)

gtest_discover_tests(LocalEventTests)

# Optional labels for filtering in ctest
set_tests_properties(${LocalEventTests_TESTS} PROPERTIES LABELS "Event")







add_executable(GlobalEventTests ${CMAKE_CURRENT_SOURCE_DIR}/Global/GlobalEventTests.cpp)
# Optional, but common: local includes

target_link_libraries(GlobalEventTests
AtlasNetCore
gtest
)

gtest_discover_tests(GlobalEventTests)

# Optional labels for filtering in ctest
set_tests_properties(${GlobalEventTests_TESTS} PROPERTIES LABELS "Event")
