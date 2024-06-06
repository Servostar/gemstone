//
// Created by servostar on 5/8/24.
//

#include <ast/ast.h>
#include <sys/log.h>
#include <mem/cache.h>

void generate_statement(const AST_NODE_PTR stmt) {
    const AST_NODE_PTR add = AST_new_node(empty_location(), AST_Add, NULL);

    AST_push_node(add, AST_new_node(empty_location(), AST_Int, "3"));
    AST_push_node(add, AST_new_node(empty_location(), AST_Int, "6"));

    AST_push_node(stmt, add);
}

void generate_branch(const AST_NODE_PTR stmt) {
    const AST_NODE_PTR branch = AST_new_node(empty_location(), AST_If, NULL);
    const AST_NODE_PTR gt = AST_new_node(empty_location(), AST_Greater, NULL);

    AST_push_node(branch, gt);

    AST_push_node(gt, AST_new_node(empty_location(), AST_Float, "2.3"));
    AST_push_node(gt, AST_new_node(empty_location(), AST_Float, "0.79"));

    AST_push_node(stmt, branch);

    generate_statement(branch);
}

int main(void) {
    mem_init();
    AST_init();

    const AST_NODE_PTR root = AST_new_node(empty_location(), AST_Stmt, NULL);

    generate_branch(root);

    FILE* output = fopen("tmp/graph.gv", "w");

    if (output == NULL) {
        PANIC("unable to open file");
    }

    AST_fprint_graphviz(output, root);

    fflush(output);

    fclose(output);

    AST_delete_node(root);

    return 0;
}
