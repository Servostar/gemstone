
#include "llvm/types/structs.h"
#include <llvm/types/scope.h>
#include <llvm/types/composite.h>
#include <ast/ast.h>
#include <llvm/types/type.h>
#include <stdlib.h>

GemstoneTypeRef get_type_from_ast(const TypeScopeRef scope, const AST_NODE_PTR type_node) {
    if (type_node->kind != AST_Type) {
        PANIC("Node must be of type AST_Type: %s", AST_node_to_string(type_node));
    }

    GemstoneTypeRef type = malloc(sizeof(GemstoneType));

    if (type_node->child_count > 1) {
        // must be composite
        type->kind = TypeComposite;
        type->specs.composite = ast_type_to_composite(scope, type_node);
    } else {
        // either custom type or box
        GemstoneTypedefRef resolved_type = type_scope_get_type_from_name(scope, AST_get_node(type_node, 0)->value);

        if (resolved_type == NULL) {
            type->kind = TypeComposite;
            type->specs.composite = ast_type_to_composite(scope, type_node);
        } else {
            free(type);
            type = resolved_type->type;
        }
    }

    return type;
}

GemstoneTypedefRef new_typedefref(GemstoneTypeRef type, const char* name) {
    GemstoneTypedefRef typedefref = malloc(sizeof(GemstoneTypedef));

    typedefref->name = name;
    typedefref->type = type;

    return typedefref;
}

void delete_type(GemstoneTypeRef typeref) {
    switch(typeref->kind) {
        case TypeReference:
            delete_type(typeref->specs.reference.type);
            break;
        case TypeComposite:
            break;
        case TypeBox:
            PANIC("NOT IMPLEMENTED");
            break;
    }

    free(typeref);
}

void delete_typedefref(GemstoneTypedefRef ref) {
    delete_type(ref->type);
}

GemstoneTypedefRef get_type_def_from_ast(const TypeScopeRef scope, const AST_NODE_PTR typdef) {
    if (typdef->kind != AST_Typedef) {
        PANIC("node must be of type AST_Typedef");
    }

    GemstoneTypeRef type = get_type_from_ast(scope, AST_get_node(typdef, 0));
    const char* name = AST_get_node(typdef, 1)->value;

    return new_typedefref(type, name);
}

LLVMTypeRef llvm_type_from_gemstone_type(LLVMContextRef context, GemstoneTypeRef type) {
    LLVMTypeRef llvmTypeRef = NULL;

    switch (type->kind) {
        case TypeComposite:
            llvmTypeRef = llvm_type_from_composite(context, &type->specs.composite);
            break;
        default:
            PANIC("NOT IMPLEMENTED");
    }

    return llvmTypeRef;
}
