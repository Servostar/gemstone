
#include "codegen/backend.h"
#include "llvm/function/function-types.h"
#include "llvm/stmt/build.h"
#include <ast/ast.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/types/scope.h>
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

GemstoneParam param_from_ast(const TypeScopeRef scope, const AST_NODE_PTR node) {
    GemstoneParam param;

    // node is expected to be a parameter
    if (node->kind != AST_Parameter) {
        PANIC("Node must be of type AST_ParamList: %s", AST_node_to_string(node));
    }

    AST_NODE_PTR qualifier_list = AST_get_node(node, 0);
    param.qualifier = io_qualifier_from_ast_list(qualifier_list);

    AST_NODE_PTR param_decl = AST_get_node(node, 1);
    AST_NODE_PTR param_type = AST_get_node(param_decl, 0);
    param.typename = get_type_from_ast(scope, param_type);
    param.name = AST_get_node(param_decl, 1)->value;

    return param;
}

GemstoneFunRef fun_from_ast(const TypeScopeRef scope, const AST_NODE_PTR node) {
    if (node->kind != AST_Fun) {
        PANIC("Node must be of type AST_Fun: %s", AST_node_to_string(node));
    }

    GemstoneFunRef function = malloc(sizeof(GemstoneFun));
    function->name = AST_get_node(node, 0)->value;
    function->params = g_array_new(FALSE, FALSE, sizeof(GemstoneParam));

    AST_NODE_PTR list = AST_get_node(node, 1);
    for (size_t i = 0; i < list->child_count; i++) {
        AST_NODE_PTR param_list = AST_get_node(list, i);

        for (size_t k = 0; k < param_list->child_count; k++) {
            AST_NODE_PTR param = AST_get_node(param_list, k);

            GemstoneParam par = param_from_ast(scope, param);

            g_array_append_val(function->params, par);
        }
    }

    // TODO: parse function body
    return function;
}

void fun_delete(const GemstoneFunRef fun) {
    g_array_free(fun->params, TRUE);
    free(fun);
}

LLVMTypeRef llvm_generate_function_signature(LLVMContextRef context, GemstoneFunRef function) {
    unsigned int param_count = function->params->len;

    LLVMTypeRef* params = malloc(sizeof(LLVMTypeRef));

    for (size_t i = 0; i < param_count; i++) {
        GemstoneParam* gem_param = ((GemstoneParam*) function->params->data) + i;
        params[i] = llvm_type_from_gemstone_type(context, gem_param->typename);
    }

    return LLVMFunctionType(LLVMVoidType(), params, param_count, 0);
}

BackendError llvm_generate_function_implementation(TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR node) {
    LLVMContextRef context = LLVMGetModuleContext(module);
    GemstoneFunRef gemstone_signature = fun_from_ast(scope, node);

    gemstone_signature->llvm_signature = llvm_generate_function_signature(context, gemstone_signature);
    gemstone_signature->llvm_function = LLVMAddFunction(module, gemstone_signature->name, gemstone_signature->llvm_signature);

    type_scope_add_fun(scope, gemstone_signature);

    LLVMBasicBlockRef llvm_body = LLVMAppendBasicBlock(gemstone_signature->llvm_function, "body");
    LLVMBuilderRef llvm_builder = LLVMCreateBuilderInContext(context);
    LLVMPositionBuilderAtEnd(llvm_builder, llvm_body);

    // create new function local scope
    TypeScopeRef local_scope = type_scope_new();
    size_t local_scope_idx = type_scope_append_scope(scope, local_scope);

    for (size_t i = 0; i < node->child_count; i++) {
        AST_NODE_PTR child_node = AST_get_node(node, i);
        if (child_node->kind == AST_StmtList) {
            llvm_build_statement_list(llvm_builder, local_scope, module, child_node);
        }
    }

    // automatic return at end of function
    LLVMBuildRetVoid(llvm_builder);

    // dispose function local scope
    type_scope_remove_scope(scope, local_scope_idx);
    type_scope_delete(local_scope);

    return SUCCESS;
}
