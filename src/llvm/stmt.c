//
// Created by servostar on 5/28/24.
//

#include <codegen/backend.h>
#include <llvm/func.h>
#include <llvm/parser.h>
#include <llvm/stmt.h>
#include <llvm/expr.h>
#include <sys/log.h>

BackendError impl_assign_stmt([[maybe_unused]] LLVMBackendCompileUnit *unit,
                              const LLVMBuilderRef builder, const LLVMLocalScope *scope,
                              const Assignment *assignment) {
    BackendError err = SUCCESS;
    DEBUG("implementing assignment for variabel: %s", assignment->variable->name);

    // TODO: resolve expression to LLVMValueRef
    const LLVMValueRef llvm_value = NULL;

    switch (assignment->variable->kind) {
        case VariableKindDeclaration:
        case VariableKindDefinition:
            const LLVMValueRef llvm_ptr =
                    get_variable(scope, assignment->variable->name);
            LLVMBuildStore(builder, llvm_value, llvm_ptr);
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
        [[maybe_unused]]
        Statement *stmt = ((Statement *) block->statemnts->data) + i;

        // TODO: implement statement
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
    impl_expr(unit, scope, builder, (Expression*) &while_stmt->conditon, &cond_result);

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

        [[maybe_unused]]
        Paramer* parameter = (Paramer*) call->function->parameter->data + i;
        // TODO: create a pointer to LLVMValueRef in case parameter is `out`

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

        err = impl_cond_block(unit, builder, scope, (Expression*) &branch->ifBranch.conditon, &branch->ifBranch.block, &cond_block,
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

        err = impl_cond_block(unit, builder, scope, &elseIf->conditon, &elseIf->block, &cond_block,
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
        LLVMBasicBlockRef next_block = ((LLVMBasicBlockRef*) cond_blocks->data)[i + 1];
        LLVMBasicBlockRef cond_block = ((LLVMBasicBlockRef*) cond_blocks->data)[i];
        LLVMBasicBlockRef body_block = ((LLVMBasicBlockRef*) body_blocks->data)[i];
        LLVMValueRef cond_value = ((LLVMValueRef*) cond_values->data)[i];

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

BackendError impl_stmt([[maybe_unused]] LLVMBackendCompileUnit *unit, [[maybe_unused]] Statement *stmt) {
    // TODO: implement
    return SUCCESS;
}
