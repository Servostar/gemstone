//
// Created by servostar on 5/28/24.
//

#include <codegen/backend.h>
#include <sys/log.h>
#include <llvm/parser.h>
#include <llvm/llvm-ir/stmt.h>
#include <llvm/llvm-ir/expr.h>
#include <llvm/llvm-ir/func.h>
#include <llvm/llvm-ir/types.h>

BackendError impl_assign_stmt(
        LLVMBackendCompileUnit *unit,
        LLVMBuilderRef
        builder,
        LLVMLocalScope *scope,
        const Assignment *assignment
) {
    BackendError err = SUCCESS;
    DEBUG("implementing assignment for variable: %s", assignment->variable->name);

    LLVMValueRef llvm_value = NULL;
    err = impl_expr(unit, scope, builder, assignment->value, &llvm_value);
    if (err.kind != Success) {
        return err;
    }

    switch (assignment->variable->kind) {
        case VariableKindDeclaration:
        case VariableKindDefinition:
            LLVMValueRef llvm_ptr =
                    get_variable(scope, assignment->variable->name);
            LLVMBuildStore(builder, llvm_value, llvm_ptr
            );
            break;
        case VariableKindBoxMember:
            // TODO: resolve LLVMValueRef from BoxAccess
            break;
    }

    return err;
}

BackendError impl_basic_block(LLVMBackendCompileUnit *unit,
                              LLVMBuilderRef builder, LLVMLocalScope *scope,
                              const Block *block, LLVMBasicBlockRef *llvm_block) {
    BackendError err = SUCCESS;

    LLVMLocalScope *block_scope = new_local_scope(scope);
    // append a new LLVM basic block
    *llvm_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                "basic block");
    LLVMPositionBuilderAtEnd(builder, *llvm_block);

    for (size_t i = 0; i < block->statemnts->len; i++) {
        Statement *stmt = ((Statement *) block->statemnts->data) + i;
        impl_stmt(unit, builder, scope, stmt);
    }

    delete_local_scope(block_scope);

    return err;
}

BackendError impl_while(LLVMBackendCompileUnit *unit,
                        LLVMBuilderRef builder, LLVMLocalScope *scope,
                        const While *while_stmt) {
    BackendError err;

    // Create condition block
    LLVMBasicBlockRef while_cond_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                                       "loop.while.cond");
    LLVMPositionBuilderAtEnd(builder, while_cond_block);
    // Resolve condition in block to a variable
    LLVMValueRef cond_result = NULL;
    impl_expr(unit, scope, builder, (Expression *) &while_stmt->conditon, &cond_result);

    // build body of loop
    LLVMBasicBlockRef while_body_block = NULL;
    err = impl_basic_block(unit, builder, scope, &while_stmt->block, &while_body_block);
    LLVMPositionBuilderAtEnd(builder, while_body_block);
    // jump back to condition after body end
    LLVMBuildBr(builder, while_cond_block);

    // builder will continue after the loop
    LLVMBasicBlockRef while_after_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                                        "loop.while.after");
    // build conditional branch at end of condition block
    LLVMPositionBuilderAtEnd(builder, while_cond_block);
    LLVMBuildCondBr(builder, cond_result, while_body_block, while_after_block);

    LLVMPositionBuilderAtEnd(builder, while_after_block);

    return err;
}

gboolean is_parameter_out(Parameter *param) {
    gboolean is_out = FALSE;

    if (param->kind == ParameterDeclarationKind) {
        is_out = param->impl.declaration.qualifier == Out || param->impl.declaration.qualifier == InOut;
    } else {
        is_out = param->impl.definiton.declaration.qualifier == Out ||
                 param->impl.definiton.declaration.qualifier == InOut;
    }

    return is_out;
}

BackendError impl_func_call(LLVMBackendCompileUnit *unit,
                            LLVMBuilderRef builder, LLVMLocalScope *scope,
                            const FunctionCall *call) {
    BackendError err = SUCCESS;

    GArray *arguments = g_array_new(FALSE, FALSE, sizeof(LLVMValueRef));

    for (size_t i = 0; i < call->expressions->len; i++) {
        Expression *arg = ((Expression *) call->expressions->data) + i;

        LLVMValueRef llvm_arg = NULL;
        err = impl_expr(unit, scope, builder, arg, &llvm_arg);
        if (err.kind != Success) {
            break;
        }

        Parameter *parameter = g_array_index(call->function->parameter, Parameter*, i);

        if (is_parameter_out(parameter)) {
            LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(unit->context), 0, 0);
            llvm_arg = LLVMBuildGEP2(builder, LLVMTypeOf(llvm_arg), llvm_arg, &zero, 1, "");
        }

        g_array_append_vals(arguments, &llvm_arg, 1);
    }

    if (err.kind == Success) {
        LLVMValueRef llvm_func = LLVMGetNamedFunction(unit->module, "");
        LLVMTypeRef llvm_func_type = LLVMTypeOf(llvm_func);
        LLVMBuildCall2(builder, llvm_func_type, llvm_func, (LLVMValueRef *) arguments->data, arguments->len,
                       "stmt.call");
    }

    g_array_free(arguments, FALSE);

    return err;
}

BackendError
impl_cond_block(LLVMBackendCompileUnit *unit, LLVMBuilderRef builder, LLVMLocalScope *scope, Expression *cond,
                const Block *block, LLVMBasicBlockRef *cond_block, LLVMBasicBlockRef *body_block,
                LLVMValueRef *llvm_cond) {
    BackendError err;

    *cond_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                "stmt.branch.cond");
    LLVMPositionBuilderAtEnd(builder, *cond_block);
    // Resolve condition in block to a variable
    err = impl_expr(unit, scope, builder, cond, llvm_cond);
    if (err.kind == Success) {
        // build body of loop
        err = impl_basic_block(unit, builder, scope, block, body_block);
    }

    return err;
}

BackendError impl_branch(LLVMBackendCompileUnit *unit,
                         LLVMBuilderRef builder, LLVMLocalScope *scope,
                         const Branch *branch) {
    BackendError err = SUCCESS;

    GArray *cond_blocks = g_array_new(FALSE, FALSE, sizeof(LLVMBasicBlockRef));
    GArray *body_blocks = g_array_new(FALSE, FALSE, sizeof(LLVMBasicBlockRef));
    GArray *cond_values = g_array_new(FALSE, FALSE, sizeof(LLVMValueRef));

    // add If to arrays
    {
        LLVMBasicBlockRef cond_block = NULL;
        LLVMBasicBlockRef body_block = NULL;
        LLVMValueRef cond_value = NULL;

        err = impl_cond_block(unit, builder, scope, (Expression *) &branch->ifBranch.conditon, &branch->ifBranch.block,
                              &cond_block,
                              &body_block, &cond_value);

        g_array_append_val(cond_blocks, cond_block);
        g_array_append_val(body_blocks, body_block);
        g_array_append_val(cond_values, cond_value);
    }

    // generate else if(s)
    for (size_t i = 0; i < branch->elseIfBranches->len; i++) {
        LLVMBasicBlockRef cond_block = NULL;
        LLVMBasicBlockRef body_block = NULL;
        LLVMValueRef cond_value = NULL;

        ElseIf *elseIf = ((ElseIf *) branch->elseIfBranches->data) + i;

        err = impl_cond_block(unit, builder, scope, elseIf->conditon, &elseIf->block, &cond_block,
                              &body_block, &cond_value);

        g_array_append_val(cond_blocks, cond_block);
        g_array_append_val(body_blocks, body_block);
        g_array_append_val(cond_values, cond_value);
    }

    // else block
    if (branch->elseBranch.nodePtr != NULL) {
        LLVMBasicBlockRef else_block = NULL;
        err = impl_basic_block(unit, builder, scope, &branch->elseBranch.block, &else_block);
        g_array_append_val(cond_blocks, else_block);
    }

    LLVMBasicBlockRef after_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                                  "stmt.branch.after");
    LLVMPositionBuilderAtEnd(builder, after_block);
    // in case no else block is present
    // make the after block the else
    if (branch->elseBranch.nodePtr == NULL) {
        g_array_append_val(cond_blocks, after_block);
    }

    for (size_t i = 0; i < cond_blocks->len - 1; i++) {
        LLVMBasicBlockRef next_block = ((LLVMBasicBlockRef *) cond_blocks->data)[i + 1];
        LLVMBasicBlockRef cond_block = ((LLVMBasicBlockRef *) cond_blocks->data)[i];
        LLVMBasicBlockRef body_block = ((LLVMBasicBlockRef *) body_blocks->data)[i];
        LLVMValueRef cond_value = ((LLVMValueRef *) cond_values->data)[i];

        LLVMPositionBuilderAtEnd(builder, cond_block);
        LLVMBuildCondBr(builder, cond_value, body_block, next_block);

        LLVMPositionBuilderAtEnd(builder, body_block);
        LLVMBuildBr(builder, after_block);
    }

    g_array_free(cond_blocks, TRUE);
    g_array_free(body_blocks, TRUE);
    g_array_free(cond_values, TRUE);

    return err;
}

BackendError impl_decl(LLVMBackendCompileUnit *unit,
                       LLVMBuilderRef builder,
                       LLVMLocalScope *scope,
                       VariableDeclaration *decl,
                       const char *name) {
    DEBUG("implementing local declaration: %s", name);
    BackendError err = SUCCESS;
    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope->func_scope->global_scope, decl->type, &llvm_type);

    if (err.kind != Success) {
        return err;
    }

    DEBUG("creating local variable...");
    LLVMValueRef local = LLVMBuildAlloca(builder, llvm_type, name);

    LLVMValueRef initial_value = NULL;
    err = get_type_default_value(unit, scope->func_scope->global_scope, decl->type, &initial_value);

    if (err.kind == Success) {
        DEBUG("setting default value...");
        LLVMBuildStore(builder, initial_value, local);
        g_hash_table_insert(scope->vars, (gpointer) name, local);
    } else {
        ERROR("unable to initialize local variable: %s", err.impl.message);
    }

    return err;
}

BackendError impl_def(LLVMBackendCompileUnit *unit,
                      LLVMBuilderRef builder,
                      LLVMLocalScope *scope,
                      VariableDefiniton *def,
                      const char *name) {
    DEBUG("implementing local definition: %s", name);
    BackendError err = SUCCESS;
    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope->func_scope->global_scope, def->declaration.type, &llvm_type);

    if (err.kind != Success) {
        return err;
    }

    LLVMValueRef initial_value = NULL;
    err = impl_expr(unit, scope, builder, def->initializer, &initial_value);
    if (err.kind != Success) {
        return err;
    }

    DEBUG("creating local variable...");
    LLVMValueRef local = LLVMBuildAlloca(builder, llvm_type, name);

    DEBUG("setting default value");
    LLVMBuildStore(builder, initial_value, local);
    g_hash_table_insert(scope->vars, (gpointer) name, local);
    return err;
}

BackendError impl_var(LLVMBackendCompileUnit *unit,
                      LLVMBuilderRef builder,
                      LLVMLocalScope *scope,
                      Variable *var) {
    BackendError err;

    switch (var->kind) {
        VariableKindDeclaration:
            err = impl_decl(unit, builder, scope, &var->impl.declaration, var->name);
            break;
        VariableKindDefinition:
            err = impl_def(unit, builder, scope, &var->impl.definiton, var->name);
            break;
        default:
            err = new_backend_impl_error(Implementation, NULL, "Unexpected variable kind in statement");
            break;
    }

    return err;
}

BackendError impl_stmt(LLVMBackendCompileUnit *unit, LLVMBuilderRef builder, LLVMLocalScope *scope, Statement *stmt) {
    BackendError err;

    switch (stmt->kind) {
        StatementKindAssignment:
            err = impl_assign_stmt(unit, builder, scope, &stmt->impl.assignment);
            break;
        StatementKindBranch:
            err = impl_branch(unit, builder, scope, &stmt->impl.branch);
            break;
        StatementKindDeclaration:
        StatementKindDefinition:
            err = impl_var(unit, builder, scope, stmt->impl.variable);
            break;
        StatementKindWhile:
            err = impl_while(unit, builder, scope, &stmt->impl.whileLoop);
            break;
        StatementKindFunctionCall:
            err = impl_func_call(unit, builder, scope, &stmt->impl.call);
            break;
        default:
            err = new_backend_impl_error(Implementation, NULL, "Unexpected statement kind");
    }

    return err;
}

BackendError impl_block(LLVMBackendCompileUnit *unit,
                        LLVMBuilderRef builder, LLVMFuncScope *scope,
                        const Block *block) {
    BackendError err = SUCCESS;

    LLVMLocalScope *function_entry_scope = malloc(sizeof(LLVMLocalScope));
    function_entry_scope->func_scope = scope;
    function_entry_scope->vars = g_hash_table_new(g_str_hash, g_str_equal);
    function_entry_scope->parent_scope = NULL;

    LLVMBasicBlockRef llvm_block = NULL;
    err = impl_basic_block(unit, builder, function_entry_scope, block, &llvm_block);

    delete_local_scope(function_entry_scope);

    return err;
}
