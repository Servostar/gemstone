include(CTest)

include_directories(${PROJECT_SOURCE_DIR}/src)

# ------------------------------------------------ #
# Setup Glib 2.0                                   #
# ------------------------------------------------ #

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
include_directories(PRIVATE ${GLIB_INCLUDE_DIRS})

# ------------------------------------------------ #
# Setup TOML-C99                                   #
# ------------------------------------------------ #

include_directories(${PROJECT_SOURCE_DIR}/dep/tomlc99)

# ------------------------------------------------------- #
# CTEST 1
# test the memory cache

add_executable(cache
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        ${PROJECT_SOURCE_DIR}/src/sys/col.c
        ${PROJECT_SOURCE_DIR}/src/cfg/opt.c
        ${PROJECT_SOURCE_DIR}/src/io/files.c
        ${PROJECT_SOURCE_DIR}/src/mem/cache.c
        cache_test.c)
set_target_properties(cache
        PROPERTIES
        OUTPUT_NAME "cache"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/cache)
target_link_libraries(cache PkgConfig::GLIB)
target_link_libraries(cache tomlc99)
add_test(NAME cache
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${GEMSTONE_BINARY_DIR}/tests/cache/cache)
