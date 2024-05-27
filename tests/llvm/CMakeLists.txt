include(CTest)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(PRIVATE ${GLIB_INCLUDE_DIRS})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
link_libraries(PkgConfig::GLIB)    

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

# ------------------------------------------------------- #
# CTEST 1
# test llvm backend codegen for global variables

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/main.c)
add_executable(global_vars
global_vars.c ${SOURCE_FILES})
set_target_properties(global_vars
        PROPERTIES
        OUTPUT_NAME "global_vars"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/llvm)
add_test(NAME global_vars
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/llvm
        COMMAND ${GEMSTONE_BINARY_DIR}/tests/llvm/global_vars)
