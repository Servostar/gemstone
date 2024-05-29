//
// Created by servostar on 5/28/24.
//

#include <codegen/backend.h>
#include <llvm/func.h>
#include <llvm/parser.h>
#include <llvm/stmt.h>
#include <llvm/expr.h>
#include <sys/log.h>

BackendError impl_assign_stmt(LLVMBackendCompileUnit *unit,
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
    impl_expr(unit, scope, builder, &while_stmt->conditon, &cond_result);

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

BackendError impl_branch(LLVMBackendCompileUnit *unit,
                         LLVMBuilderRef builder, LLVMLocalScope *scope,
                         const Branch *branch) {
    BackendError err = SUCCESS;

    LLVMBasicBlockRef if_cond_block = LLVMAppendBasicBlockInContext(unit->context, scope->func_scope->llvm_func,
                                                                       "stmt.branch.cond");
    // Resolve condition in block to a variable
    LLVMValueRef cond_result = NULL;
    impl_expr(unit, scope, builder, &branch->ifBranch.conditon, &cond_result);

    return err;
}

BackendError impl_stmt(LLVMBackendCompileUnit *unit, Statement *stmt) {}
