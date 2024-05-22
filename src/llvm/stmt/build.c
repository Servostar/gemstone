
#include "llvm/function/function-types.h"
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/decl/variable.h>
#include <llvm/types/scope.h>
#include <ast/ast.h>
#include <llvm/stmt/build.h>
#include <llvm/expr/build.h>
#include <sys/log.h>

BackendError llvm_build_statement(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR stmt_node) {
    switch (stmt_node->kind) {
        case AST_Decl: {
                GArray* decls = declaration_from_ast(scope, stmt_node);
        
                for (size_t i = 0; i < decls->len; i++) {
                    GemstoneDeclRef decl = ((GemstoneDeclRef*) decls->data)[i];
                
                    BackendError err = llvm_create_declaration(module, builder, decl, &decl->llvm_value);

                    if (err.kind != Success)
                        break;

                    type_scope_add_variable(scope, decl);
                }

                // TODO: make sure all decls are freed later
                g_array_free(decls, FALSE);
            }
            break;
        case AST_Assign: {
                AST_NODE_PTR variable_name = AST_get_node(stmt_node, 0);
                AST_NODE_PTR expression = AST_get_node(stmt_node, 1);

                LLVMValueRef yield = NULL;
                BackendError err = llvm_build_expression(builder, scope, module, expression, &yield);

                GemstoneDeclRef variable = type_scope_get_variable(scope, variable_name->value);

                LLVMBuildStore(builder, yield, variable->llvm_value);
            }
            break;
        case AST_Stmt:
            llvm_build_statement(builder, scope, module, stmt_node);
            break;
        case AST_Call: {
                AST_NODE_PTR name = AST_get_node(stmt_node, 0);
                AST_NODE_PTR expr_list = AST_get_node_by_kind(stmt_node, AST_ExprList);
                GemstoneFunRef function_signature = type_scope_get_fun_from_name(scope, name->value);
                size_t arg_count = function_signature->params->len;
                
                LLVMValueRef* args = NULL;
                BackendError err = llvm_build_expression_list(builder, scope, module, expr_list, &args);

                LLVMBuildCall2(builder, function_signature->llvm_signature, function_signature->llvm_function, args, arg_count, name->value);
            }
            break;
        case AST_Def:
            // TODO: implement definition
            break;
        case AST_While:
            // TODO: implement while
            break;
        case AST_If:
            // TODO: implement if
            break;
        case AST_IfElse:
            // TODO: implement else if
            break;
        case AST_Else:
            // TODO: implement else
            break;
        default:
            ERROR("Invalid AST node: %s", AST_node_to_string(stmt_node));
            return new_backend_impl_error(Implementation, stmt_node, "AST is invalid");
    }

    return SUCCESS;
}

BackendError llvm_build_statement_list(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR node) {
    for (size_t i = 0; i < node->child_count; i++) {
        AST_NODE_PTR stmt_node = AST_get_node(node, i);

        llvm_build_statement(builder, scope, module, stmt_node);
    }
}
