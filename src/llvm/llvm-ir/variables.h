
#ifndef LLVM_BACKEND_VARIABLES_H_
#define LLVM_BACKEND_VARIABLES_H_

#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/parser.h>
#include <set/types.h>

BackendError impl_global_variables(LLVMBackendCompileUnit* unit,
                                   LLVMGlobalScope* scope,
                                   GHashTable* variables);

LLVMValueRef get_global_variable(LLVMGlobalScope* scope, char* name);

#endif  // LLVM_BACKEND_VARIABLES_H_
