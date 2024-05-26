
#ifndef LLVM_BACKEND_TYPES_H_
#define LLVM_BACKEND_TYPES_H_

#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/parser.h>
#include <set/types.h>

BackendError impl_types(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                        GHashTable* types);

BackendError get_type_impl(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                           Type* gemstone_type, LLVMTypeRef* llvm_type);

BackendError get_type_default_value(LLVMBackendCompileUnit* unit,
                                    LLVMGlobalScope* scope, Type* gemstone_type,
                                    LLVMValueRef* llvm_value);

#endif  // LLVM_BACKEND_TYPES_H_
