#include "llvm/types/structs.h"
#include <llvm/types/type.h>
#include <ast/ast.h>
#include <llvm/decl/variable.h>
#include <sys/log.h>
#include <stdlib.h>

static StorageQualifier get_storage_qualifier_from_ast(AST_NODE_PTR storageQualifierNode) {
    if (storageQualifierNode->kind != AST_Storage) {
        PANIC("Node must be of type AST_Fun: %s", AST_node_to_string(storageQualifierNode));
    }

    StorageQualifier storageQualifier;

    if (strcmp(storageQualifierNode->value, "local") == 0) {
        storageQualifier = StorageQualifierLocal;
    } else if (strcmp(storageQualifierNode->value, "static") == 0) {
        storageQualifier = StorageQualifierStatic;
    } else if (strcmp(storageQualifierNode->value, "global") == 0) {
        storageQualifier = StorageQualifierGlobal;
    } else {
        PANIC("unknown storage qualifier: %s", storageQualifierNode->value);
    }

    return storageQualifier;
}

GArray* declaration_from_ast(TypeScopeRef scope, const AST_NODE_PTR node) {
    if (node->kind != AST_Decl) {
        PANIC("Node must be of type AST_Fun: %s", AST_node_to_string(node));
    }

    GArray* decls = g_array_new(FALSE, FALSE, sizeof(GemstoneDeclRef));

    AST_NODE_PTR first_child = AST_get_node(node, 0);
    
    StorageQualifier qualifier = StorageQualifierLocal;
    GemstoneTypeRef type = NULL;
    AST_NODE_PTR list = NULL;

    if (first_child->kind == AST_Storage) {
        qualifier = get_storage_qualifier_from_ast(first_child);
        type = get_type_from_ast(scope, AST_get_node(node, 1));
        list = AST_get_node(node, 2);
    
    } else {
        type = get_type_from_ast(scope, first_child);
        list = AST_get_node(node, 1);
    }

    if (list->kind != AST_IdentList) {
        PANIC("Node must be of type AST_IdentList: %s", AST_node_to_string(node));
    }

    for (size_t i = 0; i < list->child_count; i++) {
        GemstoneDeclRef ref = malloc(sizeof(GemstoneDecl));
        ref->name = AST_get_node(list, i)->value;
        ref->storageQualifier = qualifier;
        ref->type = type;

        g_array_append_val(decls, ref);
    }

    return decls;
}
