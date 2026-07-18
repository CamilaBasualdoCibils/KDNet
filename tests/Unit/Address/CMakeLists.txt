




add_executable(AtlasNet_tests_Address ${CMAKE_CURRENT_SOURCE_DIR}/Address.cpp)
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_Address
AtlasNetTestCrashUtils
gtest
)

gtest_discover_tests(AtlasNet_tests_Address)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_Address_TESTS} PROPERTIES LABELS "Address")
