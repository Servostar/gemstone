cmake_minimum_required(VERSION 3.15...3.25)

project(gemstone
        VERSION 0.1.0
        DESCRIPTION "programming language compiler lld c++ layer"
        LANGUAGES CXX)

set(GEMSTONE_BINARY_DIR ${PROJECT_SOURCE_DIR}/../../bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------ #
#  LLVM                                            #
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

file(GLOB_RECURSE SOURCE_FILES *.cpp)

add_library(lldcl ${SOURCE_FILES})
target_link_libraries(lldcl)
set_target_properties(lldcl
        PROPERTIES
        OUTPUT_NAME "lldcl"
        ARCHIVE_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/dep)
