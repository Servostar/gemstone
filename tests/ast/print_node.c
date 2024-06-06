//
// Created by servostar on 5/7/24.
//

#include <ast/ast.h>
#include <mem/cache.h>

int main(void) {
    mem_init();
    AST_init();

    const AST_NODE_PTR node = AST_new_node(empty_location(), 0, "value");

    for (size_t i = 0; i < AST_ELEMENT_COUNT; i++) {
        // set kind
        node->kind = i;
        // print symbol
        printf("%ld %s\n", i, AST_node_to_string(node));
    }

    AST_delete_node(node);

    return 0;
}
