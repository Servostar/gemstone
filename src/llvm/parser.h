
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
    GHashTable* variables;
    GHashTable* functions;
} LLVMGlobalScope;

LLVMGlobalScope* new_global_scope();

void delete_global_scope(LLVMGlobalScope* scope);

typedef struct LLVMLocalScope_t LLVMLocalScope;

typedef struct LLVMLocalScope_t {
    LLVMGlobalScope* global_scope;
    LLVMLocalScope* parent_scope;
    GHashTable* params;
    GHashTable* variables;
} LLVMLocalScope;

LLVMLocalScope* new_local_scope(LLVMGlobalScope* global_scope, LLVMLocalScope* parent_scope);

void delete_local_scope(LLVMLocalScope* scope);

BackendError parse_module(const Module* module, void**);

#endif // LLVM_BACKEND_PARSE_H_
