//
// Created by servostar on 5/28/24.
//

#ifndef LLVM_BACKEND_STMT_H
#define LLVM_BACKEND_STMT_H

#include <llvm/llvm-ir/func.h>

BackendError impl_block(LLVMBackendCompileUnit *unit,
                              LLVMBuilderRef builder, LLVMFuncScope *scope,
                              const Block *block);

BackendError impl_stmt(LLVMBackendCompileUnit *unit, LLVMBuilderRef builder, LLVMLocalScope *scope, Statement *stmt);

#endif // LLVM_BACKEND_STMT_H
