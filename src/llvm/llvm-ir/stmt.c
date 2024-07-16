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
#include <assert.h>
#include <mem/cache.h>

BackendError impl_param_load(
        LLVMBackendCompileUnit *unit,
        LLVMBuilderRef builder,
        LLVMLocalScope *scope,
        const StorageExpr *expr,
        LLVMValueRef* storage_target) {
    BackendError err = SUCCESS;

    if (expr->impl.parameter->impl.declaration.qualifier == Out || expr->impl.parameter->impl.declaration.qualifier == InOut) {
        LLVMTypeRef llvm_type = NULL;
        err = get_type_impl(unit, scope->func_scope->global_scope, expr->impl.parameter->impl.declaration.type, &llvm_type);
        if (err.kind != Success) {
            return err;
        }

        *storage_target = LLVMBuildLoad2(builder, llvm_type, *storage_target, "strg.param.out.load");
    }

    return err;
}

BackendError impl_storage_expr(
        LLVMBackendCompileUnit *unit,
        LLVMBuilderRef
        builder,
        LLVMLocalScope *scope,
        const StorageExpr *expr,
        LLVMValueRef* storage_target) {

    BackendError err = SUCCESS;

    switch (expr->kind) {
        case StorageExprKindVariable:
            *storage_target =
                    get_variable(scope, expr->impl.variable->name);
            break;
        case StorageExprKindParameter:
            *storage_target =
                    get_parameter(scope->func_scope, expr->impl.parameter->name);
            break;
        case StorageExprKindDereference:

            LLVMValueRef index = NULL;
            err = impl_expr(unit, scope, builder, expr->impl.dereference.index, false, &index);
            if (err.kind != Success) {
                return err;
            }

            LLVMValueRef array = NULL;
            err = impl_storage_expr(unit, builder, scope, expr->impl.dereference.array, &array);
            if (err.kind != Success) {
                return err;
            }

            if (expr->impl.dereference.array->kind == StorageExprKindParameter) {
                err = impl_param_load(unit, builder, scope, expr->impl.dereference.array, &array);
                if (err.kind != Success) {
                    return err;
                }
            }

            if (expr->impl.dereference.array->kind == StorageExprKindDereference) {
                LLVMTypeRef deref_type = NULL;
                err = get_type_impl(unit, scope->func_scope->global_scope, expr->impl.dereference.array->target_type, &deref_type);
                if (err.kind != Success) {
                    return err;
                }

                array = LLVMBuildLoad2(builder, deref_type, array, "strg.deref.indirect-load");
            }

            LLVMTypeRef deref_type = NULL;
            err = get_type_impl(unit, scope->func_scope->global_scope, expr->target_type, &deref_type);
            if (err.kind != Success) {
                return err;
            }

            *storage_target = LLVMBuildGEP2(builder, deref_type, array, &index, 1, "strg.deref");

            break;
        case StorageExprKindBoxAccess:
            // TODO: resolve LLVMValueRef from BoxAccess
            break;
    }

    return err;
}

BackendError impl_assign_stmt(
        LLVMBackendCompileUnit *unit,
        LLVMBuilderRef
        builder,
        LLVMLocalScope *scope,
        const Assignment *assignment
) {
    BackendError err = SUCCESS;
    DEBUG("implementing assignment for variable: %p", assignment);

    LLVMValueRef llvm_value = NULL;
    err = impl_expr(unit, scope, builder, assignment->value, false, &llvm_value);
    if (err.kind != Success) {
        return err;
    }

    LLVMValueRef llvm_array = NULL;
    err = impl_storage_expr(unit, builder, scope, assignment->destination, &llvm_array);
    if (err.kind != Success) {
        return err;
    }

    LLVMBuildStore(builder, llvm_value, llvm_array);

    return err;
}

BackendError impl_basic_block(LLVMBackendCompileUnit *unit,
                              LLVMBuilderRef builder, LLVMLocalScope *scope,
                              const Block *block, LLVMBasicBlockRef *llvm_start_block, LLVMBasicBlockRef *llvm_end_block) {
    DEBUG("implementing basic block...");
    BackendError err = SUCCESS;

    LLVMLocalScope *block_scope = new_local_scope(scope);
    // append a new LLVM basic block
    *llvm_start_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                "stmt.block.start");
    LLVMPositionBuilderAtEnd(builder, *llvm_start_block);

    LLVMBasicBlockRef end_previous_block = *llvm_start_block;

    for (size_t i = 0; i < block->statemnts->len; i++) {
        DEBUG("building block statement %d of %d", i, block->statemnts->len);
        Statement* stmt = g_array_index(block->statemnts, Statement*, i);

        LLVMBasicBlockRef llvm_next_start_block = NULL;
        LLVMBasicBlockRef llvm_next_end_block = NULL;
        err = impl_stmt(unit, builder, scope, stmt, &llvm_next_start_block, &llvm_next_end_block);
        if (err.kind != Success) {
            return err;
        }

        if (llvm_next_end_block != NULL) {
            LLVMPositionBuilderAtEnd(builder, end_previous_block);
            LLVMBuildBr(builder, llvm_next_start_block);
            LLVMPositionBuilderAtEnd(builder, llvm_next_end_block);
            end_previous_block = llvm_next_end_block;
        }
    }

    *llvm_end_block = end_previous_block;

    delete_local_scope(block_scope);

    return err;
}

BackendError impl_while(LLVMBackendCompileUnit *unit,
                        LLVMBuilderRef builder, LLVMLocalScope *scope,
                        LLVMBasicBlockRef* llvm_start_block,
                        LLVMBasicBlockRef* llvm_end_block,
                        const While *while_stmt) {
    DEBUG("implementing while...");
    BackendError err;

    // Create condition block
    LLVMBasicBlockRef while_cond_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                                       "loop.while.cond");
    *llvm_start_block = while_cond_block;
    LLVMPositionBuilderAtEnd(builder, while_cond_block);
    // Resolve condition in block to a variable
    LLVMValueRef cond_result = NULL;
    err = impl_expr(unit, scope, builder, (Expression *) while_stmt->conditon, FALSE, &cond_result);
    if (err.kind != Success) {
        return err;
    }

    // build body of loop
    LLVMBasicBlockRef while_start_body_block = NULL;
    LLVMBasicBlockRef while_end_body_block = NULL;
    err = impl_basic_block(unit, builder, scope, &while_stmt->block, &while_start_body_block, &while_end_body_block);
    if (err.kind != Success) {
        return err;
    }

    LLVMPositionBuilderAtEnd(builder, while_end_body_block);
    // jump back to condition after body end
    LLVMBuildBr(builder, while_cond_block);

    // builder will continue after the loop
    LLVMBasicBlockRef while_after_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                                        "loop.while.after");
    // build conditional branch at end of condition block
    LLVMPositionBuilderAtEnd(builder, while_cond_block);
    LLVMBuildCondBr(builder, cond_result, while_start_body_block, while_after_block);

    LLVMPositionBuilderAtEnd(builder, while_after_block);

    *llvm_end_block = while_after_block;

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
    DEBUG("implementing function call...");
    BackendError err = SUCCESS;

    LLVMValueRef* arguments = mem_alloc(MemoryNamespaceLlvm, sizeof(LLVMValueRef) * call->expressions->len);

    for (size_t i = 0; i < call->expressions->len; i++) {
        Expression *arg = g_array_index(call->expressions, Expression*, i);

        GArray* param_list;
        if (call->function->kind == FunctionDeclarationKind) {
            param_list = call->function->impl.definition.parameter;
        } else {
            param_list = call->function->impl.declaration.parameter;
        }

        Parameter param = g_array_index(param_list, Parameter, i);

        LLVMValueRef llvm_arg = NULL;
        err = impl_expr(unit, scope, builder, arg, is_parameter_out(&param), &llvm_arg);

        if (err.kind != Success) {
            break;
        }

        if (is_parameter_out(&param)) {
            if ((arg->kind == ExpressionKindParameter && !is_parameter_out(arg->impl.parameter)) || arg->kind != ExpressionKindParameter) {
                LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), 0, false);
                LLVMTypeRef llvm_type = NULL;
                get_type_impl(unit, scope->func_scope->global_scope, param.impl.declaration.type, &llvm_type);
                llvm_arg = LLVMBuildGEP2(builder, llvm_type, llvm_arg, &index, 1, "");
            }
        }

        arguments[i] = llvm_arg;
    }

    if (err.kind == Success) {
        LLVMValueRef llvm_func = LLVMGetNamedFunction(unit->module, call->function->name);

        if (llvm_func == NULL) {
            return new_backend_impl_error(Implementation, NULL, "no declared function");
        }

        LLVMTypeRef llvm_func_type = g_hash_table_lookup(scope->func_scope->global_scope->functions, call->function->name);

        LLVMBuildCall2(builder, llvm_func_type, llvm_func, arguments, call->expressions->len,
                       "");
    }

    return err;
}

BackendError
impl_cond_block(LLVMBackendCompileUnit *unit, LLVMBuilderRef builder, LLVMLocalScope *scope, Expression *cond,
                const Block *block, LLVMBasicBlockRef *cond_block, LLVMBasicBlockRef *start_body_block, LLVMBasicBlockRef *end_body_block,
                LLVMValueRef *llvm_cond) {
    BackendError err;

    *cond_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                "stmt.branch.cond");
    LLVMPositionBuilderAtEnd(builder, *cond_block);
    // Resolve condition in block to a variable
    err = impl_expr(unit, scope, builder, cond, FALSE, llvm_cond);
    if (err.kind == Success) {
        // build body of loop
        err = impl_basic_block(unit, builder, scope, block, start_body_block, end_body_block);
    }

    return err;
}

BackendError impl_branch(LLVMBackendCompileUnit *unit,
                         LLVMBuilderRef builder, LLVMLocalScope *scope,
                         LLVMBasicBlockRef* branch_start_block,
                         LLVMBasicBlockRef* branch_end_block,
                         const Branch *branch) {
    BackendError err = SUCCESS;

    GArray *cond_blocks = g_array_new(FALSE, FALSE, sizeof(LLVMBasicBlockRef));
    GArray *start_body_blocks = g_array_new(FALSE, FALSE, sizeof(LLVMBasicBlockRef));
    GArray *end_body_blocks = g_array_new(FALSE, FALSE, sizeof(LLVMBasicBlockRef));
    GArray *cond_values = g_array_new(FALSE, FALSE, sizeof(LLVMValueRef));

    // add If to arrays
    {
        LLVMBasicBlockRef cond_block = NULL;
        LLVMBasicBlockRef start_body_block = NULL;
        LLVMBasicBlockRef end_body_block = NULL;
        LLVMValueRef cond_value = NULL;

        err = impl_cond_block(unit, builder, scope, branch->ifBranch.conditon, &branch->ifBranch.block,
                              &cond_block,
                              &start_body_block, &end_body_block, &cond_value);

        g_array_append_val(cond_blocks, cond_block);
        g_array_append_val(start_body_blocks, start_body_block);
        g_array_append_val(end_body_blocks, end_body_block);
        g_array_append_val(cond_values, cond_value);
    }

    // generate else if(s)
    if (branch->elseIfBranches != NULL) {
        for (size_t i = 0; i < branch->elseIfBranches->len; i++) {
            LLVMBasicBlockRef cond_block = NULL;
            LLVMBasicBlockRef start_body_block = NULL;
            LLVMBasicBlockRef end_body_block = NULL;
            LLVMValueRef cond_value = NULL;

            ElseIf *elseIf = ((ElseIf *) branch->elseIfBranches->data) + i;

            err = impl_cond_block(unit, builder, scope, elseIf->conditon, &elseIf->block, &cond_block,
                                  &start_body_block, &end_body_block, &cond_value);

            g_array_append_val(cond_blocks, cond_block);
            g_array_append_val(start_body_blocks, start_body_block);
            g_array_append_val(end_body_blocks, end_body_block);
            g_array_append_val(cond_values, cond_value);
        }
    }

    LLVMBasicBlockRef after_block = NULL;

    // else block
    if (branch->elseBranch.block.statemnts != NULL) {
        LLVMBasicBlockRef start_else_block = NULL;
        err = impl_basic_block(unit, builder, scope, &branch->elseBranch.block, &start_else_block, &after_block);
        g_array_append_val(cond_blocks, start_else_block);
    }

    if (after_block == NULL) {
        after_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                    "stmt.branch.after");
    }

    LLVMPositionBuilderAtEnd(builder, after_block);
    // in case no else block is present
    // make the after block the else
    if (branch->elseBranch.block.statemnts == NULL) {
        g_array_append_val(cond_blocks, after_block);
    }

    for (size_t i = 0; i < cond_blocks->len - 1; i++) {
        LLVMBasicBlockRef next_block = g_array_index(cond_blocks, LLVMBasicBlockRef, i + 1);
        LLVMBasicBlockRef cond_block =  g_array_index(cond_blocks, LLVMBasicBlockRef, i);
        LLVMBasicBlockRef start_body_block =  g_array_index(start_body_blocks, LLVMBasicBlockRef, i);
        LLVMBasicBlockRef end_body_block = g_array_index(end_body_blocks, LLVMBasicBlockRef, i);
        LLVMValueRef cond_value = g_array_index(cond_values, LLVMValueRef, i);

        LLVMPositionBuilderAtEnd(builder, cond_block);
        LLVMBuildCondBr(builder, cond_value, start_body_block, next_block);

        LLVMPositionBuilderAtEnd(builder, end_body_block);
        LLVMBuildBr(builder, after_block);
    }

    *branch_start_block = g_array_index(cond_blocks, LLVMBasicBlockRef, 0);
    *branch_end_block = after_block;

    g_array_free(cond_blocks, TRUE);
    g_array_free(start_body_blocks, TRUE);
    g_array_free(end_body_blocks, TRUE);
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
    err = impl_expr(unit, scope, builder, def->initializer, FALSE, &initial_value);
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
        case VariableKindDeclaration:
            err = impl_decl(unit, builder, scope, &var->impl.declaration, var->name);
            break;
        case VariableKindDefinition:
            err = impl_def(unit, builder, scope, &var->impl.definiton, var->name);
            break;
        default:
            err = new_backend_impl_error(Implementation, NULL, "Unexpected variable kind in statement");
            break;
    }

    return err;
}

BackendError impl_stmt(LLVMBackendCompileUnit *unit, LLVMBuilderRef builder, LLVMLocalScope *scope, Statement *stmt, LLVMBasicBlockRef* llvm_start_block, LLVMBasicBlockRef* llvm_end_block) {
    assert(stmt != NULL);
    DEBUG("implementing statement: %ld", stmt->kind);
    BackendError err;

    switch (stmt->kind) {
        case StatementKindAssignment:
            err = impl_assign_stmt(unit, builder, scope, &stmt->impl.assignment);
            break;
        case StatementKindBranch:
            err = impl_branch(unit, builder, scope, llvm_start_block, llvm_end_block, &stmt->impl.branch);
            break;
        case StatementKindDeclaration:
        case StatementKindDefinition:
            err = impl_var(unit, builder, scope, stmt->impl.variable);
            break;
        case StatementKindWhile:
            err = impl_while(unit, builder, scope, llvm_start_block, llvm_end_block, &stmt->impl.whileLoop);
            break;
        case StatementKindFunctionCall:
            err = impl_func_call(unit, builder, scope, &stmt->impl.call);
            break;
        default:
            err = new_backend_impl_error(Implementation, NULL, "Unexpected statement kind");
            break;
    }

    return err;
}

BackendError impl_block(LLVMBackendCompileUnit *unit,
                        LLVMBuilderRef builder, LLVMFuncScope *scope,
                        LLVMBasicBlockRef* llvm_start_block,
                        LLVMBasicBlockRef* llvm_end_block,
                        const Block *block) {
    DEBUG("Implementing function block...");
    BackendError err = SUCCESS;

    LLVMLocalScope *function_entry_scope = malloc(sizeof(LLVMLocalScope));
    function_entry_scope->func_scope = scope;
    function_entry_scope->vars = g_hash_table_new(g_str_hash, g_str_equal);
    function_entry_scope->parent_scope = NULL;

    err = impl_basic_block(unit, builder, function_entry_scope, block, llvm_start_block, llvm_end_block);

    delete_local_scope(function_entry_scope);

    return err;
}
