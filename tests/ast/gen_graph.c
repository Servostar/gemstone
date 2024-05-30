//
// Created by servostar on 5/7/24.
//

#include <ast/ast.h>
#include <sys/log.h>

int main(void) {

    struct AST_Node_t* node = AST_new_node(empty_location(), AST_If, NULL);

    struct AST_Node_t* child = AST_new_node(empty_location(), AST_Add, NULL);
    AST_push_node(child, AST_new_node(empty_location(), AST_Int, "43"));
    AST_push_node(child, AST_new_node(empty_location(), AST_Int, "9"));

    AST_push_node(node, child);
    AST_push_node(node, AST_new_node(empty_location(), AST_Expr, NULL));
    AST_push_node(node, AST_new_node(empty_location(), AST_Expr, NULL));

    FILE* out = fopen("ast.gv", "w+");
    // convert this file        ^^^^^^
    // to an svg with: `dot -Tsvg ast.gv > graph.svg`

    AST_fprint_graphviz(out, node);

    AST_delete_node(node);

    fflush(out);
    fclose(out);

    return 0;
}
