//
// Created by servostar on 5/7/24.
//

#include <ast/ast.h>
#include <sys/log.h>

void generate_statement(const AST_NODE_PTR stmt) {
    const AST_NODE_PTR add = AST_new_node(AST_Add, NULL);

    AST_push_node(add, AST_new_node(AST_Int, "3"));
    AST_push_node(add, AST_new_node(AST_Int, "6"));

    AST_push_node(stmt, add);
}

void generate_branch(const AST_NODE_PTR stmt) {
    const AST_NODE_PTR branch = AST_new_node(AST_If, NULL);
    const AST_NODE_PTR gt = AST_new_node(AST_Greater, NULL);

    AST_push_node(branch, gt);

    AST_push_node(gt, AST_new_node(AST_Float, "2.3"));
    AST_push_node(gt, AST_new_node(AST_Float, "0.79"));

    AST_push_node(stmt, branch);

    generate_statement(branch);
}

int main(void) {

    const AST_NODE_PTR root = AST_new_node(AST_Stmt, NULL);

    generate_branch(root);

    AST_delete_node(root);

    return 0;
}
