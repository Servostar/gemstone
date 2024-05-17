include(CTest)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/dep)
include_directories(${PROJECT_SOURCE_DIR}/src)

# ------------------------------------------------------- #
# CTEST 1
# test build configuration and dependency of Klib

add_executable(klib_hashmap
        hashmap.c)
set_target_properties(klib_hashmap
        PROPERTIES
        OUTPUT_NAME "klib_hashmap"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/klib)
target_link_libraries(klib_hashmap klib)
target_compile_options(klib_hashmap PUBLIC -Wall -Wextra -Wpedantic -Werror)
 add_test(NAME klib_hashmap
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${GEMSTONE_BINARY_DIR}/tests/klib/klib_hashmap)
