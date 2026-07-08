




add_executable(AtlasNet_tests_Tasks ${CMAKE_CURRENT_SOURCE_DIR}/Tasks.cpp)
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_Tasks
AtlasNetCore
gtest
)

gtest_discover_tests(AtlasNet_tests_Tasks)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_Tasks_TESTS} PROPERTIES LABELS "Tasks")
