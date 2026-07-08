



file(GLOB_RECURSE _Heuristic_Sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(AtlasNet_tests_Heuristic ${_Heuristic_Sources})
# Optional, but common: local includes

target_link_libraries(AtlasNet_tests_Heuristic
AtlasNetTestCrashUtils
AtlasNetTestSceneView
gtest
)

gtest_discover_tests(AtlasNet_tests_Heuristic)

# Optional labels for filtering in ctest
set_tests_properties(${AtlasNet_tests_Heuristic_TESTS} PROPERTIES LABELS "Heuristic")
