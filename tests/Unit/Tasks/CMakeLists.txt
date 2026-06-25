




add_executable(TasksTests ${CMAKE_CURRENT_SOURCE_DIR}/Tasks.cpp)
# Optional, but common: local includes

target_link_libraries(TasksTests
AtlasNetCore
gtest
)

gtest_discover_tests(TasksTests)

# Optional labels for filtering in ctest
set_tests_properties(${TasksTests_TESTS} PROPERTIES LABELS "Tasks")
