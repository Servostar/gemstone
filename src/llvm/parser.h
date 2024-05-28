
#ifndef LLVM_BACKEND_PARSE_H_
#define LLVM_BACKEND_PARSE_H_

#include <set/types.h>
#include <codegen/backend.h>
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>

typedef struct LLVMBackendCompileUnit_t {
    LLVMContextRef context;
    LLVMModuleRef module;
} LLVMBackendCompileUnit;

typedef struct LLVMGlobalScope_t {
    GHashTable* types;
    // of type LLVMValueRef
    GHashTable* variables;
    // of type LLVMTypeRef
    GHashTable* functions;
} LLVMGlobalScope;

LLVMGlobalScope* new_global_scope();

void delete_global_scope(LLVMGlobalScope* scope);

BackendError parse_module(const Module* module, void**);

#endif // LLVM_BACKEND_PARSE_H_
