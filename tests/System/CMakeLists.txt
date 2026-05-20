



add_executable(SystemTests ${CMAKE_CURRENT_SOURCE_DIR}/System.cpp)
target_compile_definitions(SystemTests
    PRIVATE
    TEST_COMPOSE_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(SystemTests
    AtlasNetTestCrashUtils
    gtest
)
add_dependencies(SystemTests AtlasNetDevDockerBuild)
gtest_discover_tests(
    SystemTests
    PROPERTIES LABELS "System"
)



#add_test(
#    NAME SystemTests
#    COMMAND ${VENV_PYTHON} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/System.py -v
#    #WORKING_DIRECTORY $<TARGET_FILE_DIR:SystemTests>  # so ./SystemTests is found
#)
#set_tests_properties(SystemTests PROPERTIES
#    ENVIRONMENT "ATLASNET_TEST_CONTROLLER_IMAGE=${ATLASNET_TEST_CONTROLLER_IMAGE}"
#)