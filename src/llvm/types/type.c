
#include <llvm/types/composite.h>
#include <ast/ast.h>
#include <llvm/types/type.h>

GemstoneType get_type_from_ast(const AST_NODE_PTR type_node) {
    if (type_node->kind != AST_Type) {
        PANIC("Node must be of type AST_Type: %s", AST_node_to_string(type_node));
    }

    GemstoneType type;

    if (type_node->child_count > 1) {
        // must be composite
        type.kind = TypeComposite;
        type.specs.composite = ast_type_to_composite(type_node);
    } else {
        // either custom type or box
        // TODO: resolve concrete type from TypeScope
    }

    return type;
}
