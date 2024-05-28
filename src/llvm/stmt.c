//
// Created by servostar on 5/28/24.
//

#include <codegen/backend.h>
#include <llvm/func.h>
#include <llvm/parser.h>
#include <llvm/stmt.h>

BackendError impl_assign_stmt(LLVMBackendCompileUnit* unit,
                              LLVMBuilderRef builder, LLVMLocalScope* scope,
                              Assignment* assignment) {
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
            break;
    }
}

BackendError impl_stmt(LLVMBackendCompileUnit* unit, Statement* stmt) {}
