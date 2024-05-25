cmake_minimum_required(VERSION 3.15...3.25)

project(gemstone_stdlib
    VERSION 0.1.0
    DESCRIPTION "gemstone programming language standard library"
    LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../bin/std")

# add native module libraries 

file(GLOB_RECURSE STDLIB_IO_SOURCE_FILES io/*.c)
add_library(io ${STDLIB_IO_SOURCE_FILES})

