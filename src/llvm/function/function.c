
#include "ast/ast.h"
#include <llvm/function/function.h>
#include <llvm/types/type.h>
#include <string.h>

static enum IO_Qualifier_t io_qualifier_from_string(const char* str) {
    if (strcmp(str, "in") == 0) {
        return In;
    }

    return Out;
}

static enum IO_Qualifier_t merge_qualifier(enum IO_Qualifier_t a, enum IO_Qualifier_t b) {
    enum IO_Qualifier_t result = Unspec;

    if (a == In && b == Out) {
        result = InOut;
    } else if (a == Out && b == In) {
        result = InOut;
    }

    return result;
}

static enum IO_Qualifier_t io_qualifier_from_ast_list(const AST_NODE_PTR node) {
    // node is expected to be a list
    if (node->kind != AST_List) {
        PANIC("Node must be of type AST_List: %s", AST_node_to_string(node));
    }

    enum IO_Qualifier_t qualifier = Unspec;

    for (size_t i = 0; i < node->child_count; i++) {
        
        AST_NODE_PTR qualifier_node = AST_get_node(node, i);

        if (qualifier_node->kind != AST_Qualifyier) {
            PANIC("Node must be of type AST_Qualifyier: %s", AST_node_to_string(node));
        }

        enum IO_Qualifier_t local_qualifier = io_qualifier_from_string(qualifier_node->value);

        if (qualifier == Unspec) {
            qualifier = local_qualifier;
        } else {
            qualifier = merge_qualifier(qualifier, local_qualifier);
        }
    }

    if (qualifier == Unspec)
        qualifier = In;

    return qualifier;
}

GemstoneParam param_from_ast(const AST_NODE_PTR node) {
    GemstoneParam param;

    // node is expected to be a parameter
    if (node->kind != AST_Parameter) {
        PANIC("Node must be of type AST_ParamList: %s", AST_node_to_string(node));
    }

    AST_NODE_PTR qualifier_list = AST_get_node(node, 0);
    param.qualifier = io_qualifier_from_ast_list(qualifier_list);

    AST_NODE_PTR param_decl = AST_get_node(node, 1);
    AST_NODE_PTR param_type = AST_get_node(param_decl, 0);
    param.typename = get_type_from_ast(param_type);
    param.name = AST_get_node(param_decl, 1)->value;

    return param;
}
