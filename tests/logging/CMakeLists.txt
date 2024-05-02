include(CTest)

# ------------------------------------------------------- #
# CTEST 1
# test the default output of the logger

add_executable(logging_output ${PROJECT_SOURCE_DIR}/src/sys/log.c output.c)
target_include_directories(logging_output PUBLIC ${PROJECT_SOURCE_DIR}/src)
set_target_properties(logging_output
        PROPERTIES
        OUTPUT_NAME "output"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/logging)
add_test(NAME logging_output WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND python ${GEMSTONE_TEST_DIR}/logging/test_logging.py check_output)

# ------------------------------------------------------- #
# CTEST 1
# test the panic functionality of the logger

add_executable(logging_panic ${PROJECT_SOURCE_DIR}/src/sys/log.c panic.c)
target_include_directories(logging_panic PUBLIC ${PROJECT_SOURCE_DIR}/src)
set_target_properties(logging_panic
        PROPERTIES
        OUTPUT_NAME "panic"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/logging)
add_test(NAME logging_panic WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND python ${GEMSTONE_TEST_DIR}/logging/test_logging.py check_panic)
