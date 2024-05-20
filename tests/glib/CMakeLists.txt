include(CTest)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(PRIVATE ${GLIB_INCLUDE_DIRS})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

# ------------------------------------------------------- #
# CTEST 1
# test Glib's hashmap

add_executable(glib_hashmap
        glib_hashmap.c)
set_target_properties(glib_hashmap
        PROPERTIES
        OUTPUT_NAME "glib_hashmap"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/glib)
target_link_libraries(glib_hashmap PkgConfig::GLIB)
add_test(NAME glib_hashmap
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${GEMSTONE_BINARY_DIR}/tests/glib/glib_hashmap)
