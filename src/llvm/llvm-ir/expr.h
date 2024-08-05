//
// Created by servostar on 5/28/24.
//

#ifndef LLVM_BACKEND_EXPR_H
#define LLVM_BACKEND_EXPR_H

#include <codegen/backend.h>
#include <llvm-c/Types.h>
#include <llvm/llvm-ir/func.h>
#include <llvm/parser.h>

BackendError impl_expr(LLVMBackendCompileUnit* unit, LLVMLocalScope* scope,
                       LLVMBuilderRef builder, Expression* expr,
                       LLVMBool reference, uint32_t deref_depth,
                       LLVMValueRef* llvm_result);

#endif // LLVM_BACKEND_EXPR_H
