include(CTest)

include_directories(${PROJECT_SOURCE_DIR}/src)

# ------------------------------------------------ #
# Setup Glib 2.0                                   #
# ------------------------------------------------ #

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

# ------------------------------------------------------- #
# CTEST 1
# test the default output of the logger

add_executable(logging_output
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        ${PROJECT_SOURCE_DIR}/src/sys/col.c
        output.c)
set_target_properties(logging_output
        PROPERTIES
        OUTPUT_NAME "output"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/logging)
add_test(NAME logging_output
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND python ${GEMSTONE_TEST_DIR}/logging/test_logging.py check_output)

# ------------------------------------------------------- #
# CTEST 2
# test the panic functionality of the logger

add_executable(logging_panic
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        ${PROJECT_SOURCE_DIR}/src/sys/col.c
        panic.c)
set_target_properties(logging_panic
        PROPERTIES
        OUTPUT_NAME "panic"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/logging)
add_test(NAME logging_panic
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND python ${GEMSTONE_TEST_DIR}/logging/test_logging.py check_panic)

# ------------------------------------------------------- #
# CTEST 3
# test the ability to write to multiple output streams

add_executable(logging_streams
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        ${PROJECT_SOURCE_DIR}/src/sys/col.c
        streams.c)
set_target_properties(logging_streams
        PROPERTIES
        OUTPUT_NAME "stream"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/logging)
add_test(NAME logging_streams
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND python ${GEMSTONE_TEST_DIR}/logging/test_logging.py check_stream)

# ------------------------------------------------------- #
# CTEST 4
# test compile time log level switch

add_executable(logging_level
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        ${PROJECT_SOURCE_DIR}/src/sys/col.c
        level.c)
set_target_properties(logging_level
        PROPERTIES
        OUTPUT_NAME "level"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/logging)
add_test(NAME logging_level
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND python ${GEMSTONE_TEST_DIR}/logging/test_logging.py check_level)
