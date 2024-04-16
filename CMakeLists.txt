cmake_minimum_required(VERSION 3.15...3.25)

# Canonical project structure
# Header must be included this way: #include <module/header.h>
#
# ├─ res
# ├─ src
# │  ├─ lex
# │  │  └─ lexer.l
# │  ├─ yacc
# │  │  └─ parser.y
# │  ├─ module
# │  │  ├─ header.h
# │  │  └─ source.c
# │  └─ main.c
# └─ tests
#    └─ test.c

project(gemstone
    VERSION 0.1.0
    DESCRIPTION "programming language compiler"
    LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------ #
#  Lex                                             #
# ------------------------------------------------ #

set(LEX_SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/lex/lexer.l)
set(LEX_GENERATED_SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/lex/lexer.ll.c)

add_custom_command(OUTPUT ${LEX_GENERATED_SOURCE_FILE}
        COMMAND lex
        ARGS -o  ${LEX_GENERATED_SOURCE_FILE} ${LEX_SOURCE_FILE}
        COMMENT "generate C source file for lexer"
        VERBATIM)

# ------------------------------------------------ #
#  Yacc                                            #
# ------------------------------------------------ #

set(YACC_SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/yacc/parser.y)
set(YACC_GENERATED_SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/yacc/parser.tab.c)

add_custom_command(OUTPUT ${YACC_GENERATED_SOURCE_FILE}
        COMMAND yacc
        ARGS -d -o ${YACC_GENERATED_SOURCE_FILE} ${YACC_SOURCE_FILE}
        COMMENT "generate C source file for parser"
        VERBATIM)

# ------------------------------------------------ #
#  Source                                          #
# ------------------------------------------------ #

file(GLOB_RECURSE SOURCE_FILES src/*.c)

# define default compile flags
set(FLAGS -Wall -Wextra -Wconversion -Wpedantic)

# ------------------------------------------------ #
#  Target RELEASE                                  #
# ------------------------------------------------ #

add_executable(release
        ${SOURCE_FILES}
        ${LEX_GENERATED_SOURCE_FILE}
        ${YACC_GENERATED_SOURCE_FILE})

set_target_properties(release
        PROPERTIES
        OUTPUT_NAME "gsc"
        RUNTIME_OUTPUT_DIRECTORY "bin/release")

# compiler flags targeting a 64-bit GCC release environment
# flags:
#  - m64: build for 64-bit
#  - O3: optimization level 3
#  - fprefetch-loop-arrays: pre load arrays used in loops by using prefetch instruction
#  - mrecip: make use RCPSS and RSQRTSS instructions
target_compile_options(release PUBLIC ${FLAGS} -m64 -O3 -fprefetch-loop-arrays -mrecip)

# add src directory as include path
target_include_directories(release PUBLIC src)

# ------------------------------------------------ #
#  Target DEBUG                                    #
# ------------------------------------------------ #

add_executable(debug
        ${SOURCE_FILES}
        ${LEX_GENERATED_SOURCE_FILE}
        ${YACC_GENERATED_SOURCE_FILE})

set_target_properties(debug
        PROPERTIES
        OUTPUT_NAME "gsc"
        RUNTIME_OUTPUT_DIRECTORY "bin/debug")

# compiler flags targeting a GCC debug environment
target_compile_options(debug PUBLIC ${FLAGS} -g)

# add src directory as include path
target_include_directories(debug PUBLIC src)

# ------------------------------------------------ #
#  Target Code Check                               #
# ------------------------------------------------ #

# Same as debug but will fail on warnings
# use as check

add_executable(check
        ${SOURCE_FILES}
        ${LEX_GENERATED_SOURCE_FILE}
        ${YACC_GENERATED_SOURCE_FILE})

set_target_properties(check
        PROPERTIES
        OUTPUT_NAME "gsc"
        RUNTIME_OUTPUT_DIRECTORY "bin/check")

# compiler flags targeting a GCC debug environment
# extra -Werror flag to treat warnings as error to make github action fail on warning
target_compile_options(check PUBLIC ${FLAGS} -g -Werror)

# add src directory as include path
target_include_directories(check PUBLIC src)

