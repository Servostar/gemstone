
#ifndef LLVM_EXPR_BUILD_H_
#define LLVM_EXPR_BUILD_H_

#include "codegen/backend.h"
#include "llvm/types/scope.h"
#include <llvm-c/Types.h>

BackendError llvm_build_expression(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR expr_node, LLVMValueRef* yield);

BackendError llvm_build_expression_list(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR expr_node, LLVMValueRef** yields);

#endif // LLVM_EXPR_BUILD_H_
