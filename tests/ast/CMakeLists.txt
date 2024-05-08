include(CTest)

include_directories(${PROJECT_SOURCE_DIR}/src)

# ------------------------------------------------------- #
# CTEST 1
# test building the syntax tree

add_executable(ast_build_tree
        ${PROJECT_SOURCE_DIR}/src/ast/ast.c
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        build_tree.c)
set_target_properties(ast_build_tree
        PROPERTIES
        OUTPUT_NAME "build_tree"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/ast)
add_test(NAME ast_build_tree
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND python ${GEMSTONE_TEST_DIR}/ast/test_ast.py check_build_tree)

# ------------------------------------------------------- #
# CTEST 2
# test node to string output

add_executable(ast_print_node
        ${PROJECT_SOURCE_DIR}/src/ast/ast.c
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        print_node.c)
set_target_properties(ast_print_node
        PROPERTIES
        OUTPUT_NAME "print_node"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/ast)
add_test(NAME ast_print_node
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND python ${GEMSTONE_TEST_DIR}/ast/test_ast.py check_print_node)

# ------------------------------------------------------- #
# CTEST 3
# test graphviz output

add_executable(ast_graphviz
        ${PROJECT_SOURCE_DIR}/src/ast/ast.c
        ${PROJECT_SOURCE_DIR}/src/sys/log.c
        print_graphviz.c)
set_target_properties(ast_graphviz
        PROPERTIES
        OUTPUT_NAME "print_graphviz"
        RUNTIME_OUTPUT_DIRECTORY ${GEMSTONE_BINARY_DIR}/tests/ast)
add_test(NAME ast_graphviz
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND python ${GEMSTONE_TEST_DIR}/ast/test_ast.py check_print_graphviz)
