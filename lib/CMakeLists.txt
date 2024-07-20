cmake_minimum_required(VERSION 3.15...3.25)

project(gemstone_stdlib
    VERSION 0.1.0
    DESCRIPTION "gemstone programming language standard library"
    LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/src)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../bin/std")

# add native module libraries 

file(GLOB_RECURSE STDLIB_IO_SOURCE_FILES src/io/*.c)
add_library(gscio ${STDLIB_IO_SOURCE_FILES})

file(GLOB_RECURSE STDLIB_MEM_SOURCE_FILES src/mem/*.c)
add_library(gscmem ${STDLIB_MEM_SOURCE_FILES})

file(GLOB_RECURSE STDLIB_OS_SOURCE_FILES src/os/*.c)
add_library(gscos ${STDLIB_OS_SOURCE_FILES})

# Complete standard library
add_library(gscstd
        ${STDLIB_IO_SOURCE_FILES}
        ${STDLIB_MEM_SOURCE_FILES}
        ${STDLIB_OS_SOURCE_FILES})
