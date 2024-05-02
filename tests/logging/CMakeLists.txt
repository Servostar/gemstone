include(CTest)

add_executable(logging ${PROJECT_SOURCE_DIR}/src/sys/log.c main.c)
target_include_directories(logging PUBLIC ${PROJECT_SOURCE_DIR}/src)
set_target_properties(logging
        PROPERTIES
        OUTPUT_NAME "output"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/logging)
add_test(NAME logging WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND python ${GEMSTONE_TEST_DIR}/logging/check_output.py)
