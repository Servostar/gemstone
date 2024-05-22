#include <llvm/types/scope.h>
#include <ast/ast.h>
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/expr/build.h>

BackendError llvm_build_arithmetic_operation(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR expr_node, enum AST_SyntaxElement_t operation, LLVMValueRef* yield) {
    AST_NODE_PTR expr_lhs = AST_get_node(expr_node, 0);
    AST_NODE_PTR expr_rhs = AST_get_node(expr_node, 1);

    LLVMValueRef llvm_lhs = NULL;
    LLVMValueRef llvm_rhs = NULL;
    BackendError err;

    err = llvm_build_expression(builder, scope, module, expr_lhs, &llvm_lhs);
    if (err.kind != Success)
        return err;

    err = llvm_build_expression(builder, scope, module, expr_rhs, &llvm_rhs);
    if (err.kind != Success)
        return err;

    switch (operation) {
        case AST_Add:
            *yield = LLVMBuildAdd(builder, llvm_lhs, llvm_rhs, "Addition");
            break;
        case AST_Sub:
            *yield = LLVMBuildSub(builder, llvm_lhs, llvm_rhs, "Subtraction");
            break;
        case AST_Mul:
            *yield = LLVMBuildMul(builder, llvm_lhs, llvm_rhs, "Multiplication");
            break;
        case AST_Div:
            *yield = LLVMBuildSDiv(builder, llvm_lhs, llvm_rhs, "Division");
            break;
        default:
            break;
    }

    return new_backend_impl_error(Implementation, expr_node, "invalid arithmetic operation");
}

BackendError llvm_build_relational_operation(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR expr_node, enum AST_SyntaxElement_t operation, LLVMValueRef* yield) {
    AST_NODE_PTR expr_lhs = AST_get_node(expr_node, 0);
    AST_NODE_PTR expr_rhs = AST_get_node(expr_node, 1);

    LLVMValueRef llvm_lhs = NULL;
    LLVMValueRef llvm_rhs = NULL;
    BackendError err;

    err = llvm_build_expression(builder, scope, module, expr_lhs, &llvm_lhs);
    if (err.kind != Success)
        return err;

    err = llvm_build_expression(builder, scope, module, expr_rhs, &llvm_rhs);
    if (err.kind != Success)
        return err;
    
    // TODO: make a difference between SignedInt, UnsignedInt and Float
    switch (operation) {
        case AST_Eq:
            *yield = LLVMBuildICmp(builder, LLVMIntEQ, llvm_lhs, llvm_rhs, "Equal");
            break;
        case AST_Greater:
            *yield = LLVMBuildICmp(builder,  LLVMIntSGT, llvm_lhs, llvm_rhs, "Greater");
            break;
        case AST_Less:
            *yield = LLVMBuildICmp(builder, LLVMIntSLT, llvm_lhs, llvm_rhs, "Less");
            break;
        default:
            break;
    }

    return new_backend_impl_error(Implementation, expr_node, "invalid arithmetic operation");
}

BackendError llvm_build_expression(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR expr_node, LLVMValueRef* yield) {

    switch (expr_node->kind) {
        case AST_Ident: {
                AST_NODE_PTR variable_name = AST_get_node(expr_node, 0);
                GemstoneDeclRef decl = type_scope_get_variable(scope, variable_name->value);
                *yield = decl->llvm_value;
            }
            break;
        case AST_Int: {
                AST_NODE_PTR constant = AST_get_node(expr_node, 0);
                // TODO: type annotation needed
                *yield = LLVMConstIntOfString(LLVMInt32Type(), constant->value, 10);
            }
        case AST_Float: {
                AST_NODE_PTR constant = AST_get_node(expr_node, 0);
                // TODO: type annotation needed
                *yield = LLVMConstRealOfString(LLVMFloatType(), constant->value);
            }
            break;
        case AST_Add:
        case AST_Sub:
        case AST_Mul:
        case AST_Div: {
                BackendError err = llvm_build_arithmetic_operation(builder, scope, module, expr_node, expr_node->kind, yield);
                if (err.kind != Success)
                    return err;
            }
        case AST_Eq:
        case AST_Greater:
        case AST_Less: {
                BackendError err = llvm_build_relational_operation(builder, scope, module, expr_node, expr_node->kind, yield);
                if (err.kind != Success)
                    return err;
            }
            break;
    }

    return SUCCESS;
}

BackendError llvm_build_expression_list(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR exprlist_node, LLVMValueRef** yields) {
    
    if (exprlist_node->kind != AST_ExprList) {
        return new_backend_impl_error(Implementation, exprlist_node, "expected expression list");
    }

    *yields = malloc(sizeof(LLVMValueRef) * exprlist_node->child_count);

    for (size_t i = 0; i < exprlist_node->child_count; i++) {
        AST_NODE_PTR expr = AST_get_node(exprlist_node, 0);

        llvm_build_expression(builder, scope, module, expr, *yields + i);
    }

    return SUCCESS;
}
