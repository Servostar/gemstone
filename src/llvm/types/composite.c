
#include "llvm/types/composite-types.h"
#include "llvm/types/scope.h"
#include "llvm/types/structs.h"
#include <llvm/types/composite.h>
#include <string.h>

LLVMTypeRef llvm_type_from_composite(LLVMContextRef context, const CompositeRef composite) {
    DEBUG("converting composite to LLVM type...");

    LLVMTypeRef type = INVALID_COMPOSITE;

    if (composite->prim == Int) {
        DEBUG("generating integer LLVM type...");

        type = LLVMIntTypeInContext(context, composite->scale * BITS_PER_BYTE);
    } else if (composite->prim == Float && composite->sign == Signed) {
        DEBUG("generating float LLVM type...");
    
        switch (composite->scale) {
            case HALF:
                type = LLVMHalfTypeInContext(context);
                break;
            case SINGLE:
                type = LLVMDoubleTypeInContext(context);
                break;
            case DOUBLE:
                type = LLVMDoubleTypeInContext(context);
                break;
            case QUAD:
                type = LLVMFP128TypeInContext(context);
                break;
            default:
                ERROR("Floating point of precision: %ld npt supported", composite->scale);
                break;
        }
    }

    return type;
}

double get_scale_factor(const char* keyword) {
    if (strcmp(keyword, "half") == 0 || strcmp(keyword, "short") == 0) {
        return 0.5;
    } else if (strcmp(keyword, "double") == 0 || strcmp(keyword, "long") == 0) {
        return 2.0;
    }

    PANIC("invalid scale factor: %s", keyword);
}

enum Scale_t collapse_scale_list(const AST_NODE_PTR list, double base) {
    double sum = base;

    for (size_t i = 0; i < list->child_count; i++) {
        AST_NODE_PTR scale = AST_get_node(list, i);

        sum *= get_scale_factor(scale->value);
    }

    if (sum >= 1.0 && sum <= 32) {
        return (enum Scale_t) sum;
    }

    PANIC("invalid combination of scale factors");
}

enum Sign_t string_to_sign(const char* keyword) {
    if (strcmp(keyword, "signed") == 0) {
        return Signed;
    } else if (strcmp(keyword, "unsigned") == 0) {
        return Unsigned;
    }

    PANIC("invalid sign: %s", keyword);
}

static enum Primitive_t resolve_primitive(const char* typename) {

    if (strcmp(typename, "int") == 0) {
        return Int;
    } else if (strcmp(typename, "float") == 0) {
        return Float;
    }

    // TODO: implement lookup of ident type
    return Int;
}

struct CompositeType_t ast_type_to_composite(const TypeScopeRef scope, AST_NODE_PTR type) {
    size_t count = type->child_count;

    struct CompositeType_t composite;
    composite.prim = Int;
    composite.scale = SINGLE;
    composite.sign = Signed;

    if (count == 1) {
        const char* typename = AST_get_node(type, 0)->value;
        GemstoneTypedefRef known_type = type_scope_get_type_from_name(scope, typename);

        if (known_type == NULL) {
            // only other type given
            composite.prim = resolve_primitive(typename);
        } else if (known_type->type->kind == TypeComposite) {
            return known_type->type->specs.composite;
        } else {
            PANIC("not a composite");
        }

    } else if (count == 2) {
        // either scale and type
        // or sign and type
        AST_NODE_PTR first_child = AST_get_node(type, 0);

        const char* typename = AST_get_node(type, 1)->value;
        GemstoneTypedefRef known_type = type_scope_get_type_from_name(scope, typename);

        if (known_type == NULL) {
            // only other type given
            composite.prim = resolve_primitive(typename);
        } else if (known_type->type->kind == TypeComposite) {
            composite.prim = known_type->type->specs.composite.prim;
            composite.scale = known_type->type->specs.composite.scale;
            composite.sign = known_type->type->specs.composite.sign;
        } else {
            PANIC("not a composite");
        }

        switch (first_child->kind) {
            case AST_List:
                composite.scale = collapse_scale_list(first_child, (double) composite.scale);
                break;
            case AST_Sign:
                composite.sign = string_to_sign(first_child->value);
                break;
            default:
                PANIC("unexpected node kind: %s", AST_node_to_string(first_child));
        }

    } else if (count == 3) {
        const char* typename = AST_get_node(type, 2)->value;
        GemstoneTypedefRef known_type = type_scope_get_type_from_name(scope, typename);

        if (known_type == NULL) {
            // only other type given
            composite.prim = resolve_primitive(typename);
        } else if (known_type->type->kind == TypeComposite) {
            composite.prim = known_type->type->specs.composite.prim;
            composite.scale = known_type->type->specs.composite.scale;
            composite.sign = known_type->type->specs.composite.sign;
        } else {
            PANIC("not a composite");
        }

        // sign, scale and type
        composite.sign = string_to_sign(AST_get_node(type, 0)->value);
        composite.scale = collapse_scale_list(AST_get_node(type, 1), (double) composite.scale);
    }

    return composite;
}

