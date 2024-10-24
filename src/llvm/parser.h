
#ifndef LLVM_BACKEND_PARSE_H_
#define LLVM_BACKEND_PARSE_H_

#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <set/types.h>

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
    // module definition
    Module* module;
} LLVMGlobalScope;

LLVMGlobalScope* new_global_scope(const Module* module);

void list_available_targets();

void delete_global_scope(LLVMGlobalScope* scope);

BackendError parse_module(const Module* module, const TargetConfig* config);

#endif // LLVM_BACKEND_PARSE_H_
