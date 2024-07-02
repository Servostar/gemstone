include(CTest)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(PRIVATE ${GLIB_INCLUDE_DIRS})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
link_libraries(PkgConfig::GLIB)

include_directories(${PROJECT_SOURCE_DIR}/dep/tomlc99)

# ------------------------------------------------ #
#  LLVM backend                                    #
# ------------------------------------------------ #

# Fetch LLVM link configuration
execute_process(COMMAND llvm-config --libs all
                OUTPUT_VARIABLE LLVM_LIBS)
# Strip whitespace from output
string(STRIP "${LLVM_LIBS}" LLVM_LIBS)
# Link all targets to LLVM
link_libraries(${LLVM_LIBS})

execute_process(COMMAND llvm-config --includedir
                OUTPUT_VARIABLE LLVM_INCLUDE_DIR)
string(STRIP "${LLVM_INCLUDE_DIR}" LLVM_INCLUDE_DIR)
include_directories(${LLVM_INCLUDE_DIR})

# ------------------------------------------------------- #
# CTEST 1
# test llvm backend codegen for global variables

#file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/llvm/*.c)
#list(REMOVE_ITEM SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/main.c)
#add_executable(global_vars
#        ${PROJECT_SOURCE_DIR}/src/codegen/backend.c
#        ${PROJECT_SOURCE_DIR}/src/sys/log.c
#        ${PROJECT_SOURCE_DIR}/src/sys/col.c
#        ${PROJECT_SOURCE_DIR}/src/set/set.c
#        ${PROJECT_SOURCE_DIR}/src/ast/ast.c
#        ${PROJECT_SOURCE_DIR}/src/cfg/opt.c
#        ${PROJECT_SOURCE_DIR}/src/io/files.c
#        ${PROJECT_SOURCE_DIR}/src/mem/cache.c
#        global_vars.c
#        ${SOURCE_FILES})
#set_target_properties(global_vars
#        PROPERTIES
#        OUTPUT_NAME "global_vars"
#        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/llvm)
#target_link_libraries(global_vars tomlc99)
#add_test(NAME global_vars
#        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/llvm
#        COMMAND ${GEMSTONE_BINARY_DIR}/tests/llvm/global_vars)
