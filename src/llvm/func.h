
#ifndef LLVM_BACKEND_FUNC_H_
#define LLVM_BACKEND_FUNC_H_

#include <llvm/parser.h>

#include <glib.h>

typedef struct LLVMFuncScope_t {
    LLVMGlobalScope* global_scope;
    // of LLVMTypeRef
    GHashTable* params;
    LLVMValueRef llvm_func;
} LLVMFuncScope;

typedef struct LLVMLocalScope_t LLVMLocalScope;

typedef struct LLVMLocalScope_t {
    // of LLVMTypeRef
    GHashTable* vars;
    LLVMFuncScope* func_scope;
    LLVMLocalScope* parent_scope;
} LLVMLocalScope;

LLVMLocalScope* new_local_scope(LLVMLocalScope* parent);

void delete_local_scope(LLVMLocalScope*);

LLVMValueRef get_variable(const LLVMLocalScope* scope, const char* name);

BackendError impl_functions(LLVMBackendCompileUnit* unit,
                                   LLVMGlobalScope* scope,
                                   GHashTable* variables);

#endif // LLVM_BACKEND_FUNC_H_
