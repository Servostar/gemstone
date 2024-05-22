
#ifndef LLVM_STMT_BUILD_H_
#define LLVM_STMT_BUILD_H_

#include <llvm/types/scope.h>
#include <codegen/backend.h>
#include <llvm-c/Types.h>

BackendError llvm_build_statement_list(LLVMBuilderRef builder, TypeScopeRef scope, LLVMModuleRef module, AST_NODE_PTR node);

#endif // LLVM_STMT_BUILD_H_
