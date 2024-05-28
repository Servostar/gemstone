//
// Created by servostar on 5/28/24.
//

#include <codegen/backend.h>
#include <llvm/func.h>
#include <llvm/parser.h>
#include <llvm/stmt.h>
#include <sys/log.h>

BackendError impl_assign_stmt(LLVMBackendCompileUnit* unit,
                              const LLVMBuilderRef builder, const LLVMLocalScope* scope,
                              const Assignment* assignment) {
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

BackendError impl_stmt(LLVMBackendCompileUnit* unit, Statement* stmt) {}
