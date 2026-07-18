




add_executable(AtlasNet_tests_RPC ${CMAKE_CURRENT_SOURCE_DIR}/RPC.cpp)
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_RPC
AtlasNetCore
gtest
)

gtest_discover_tests(AtlasNet_tests_RPC)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_RPC_TESTS} PROPERTIES LABELS "RPC")
