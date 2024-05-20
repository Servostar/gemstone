include(CTest)

# ------------------------------------------------ #
# Setup Glib 2.0                                   #
# ------------------------------------------------ #

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
    
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

# ------------------------------------------------ #
#  Source                                          #
# ------------------------------------------------ #

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(PRIVATE ${GLIB_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/main.c)

set(LEX_GENERATED_SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/lex/lexer.ll.c)
set(YACC_GENERATED_SOURCE_FILE ${PROJECT_SOURCE_DIR}/src/yacc/parser.tab.c)

# ------------------------------------------------------- #
# CTEST 1
# test typedef

add_executable(typedef
        ${SOURCE_FILES}
        ${LEX_GENERATED_SOURCE_FILE}
        ${YACC_GENERATED_SOURCE_FILE}
        typedef.c)
set_target_properties(typedef
        PROPERTIES
        OUTPUT_NAME "typedef"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/llvm)
target_link_libraries(typedef PkgConfig::GLIB)
add_test(NAME typedef
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tests/llvm/typedef
        COMMAND ${GEMSTONE_BINARY_DIR}/tests/llvm/typedef test.txt)
