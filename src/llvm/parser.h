
#include "set/types.h"
#include <codegen/backend.h>
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>

typedef struct LLVMBackendCompileUnit_t {
    LLVMContextRef context;
    LLVMModuleRef module;
} LLVMBackendCompileUnit;

BackendError parse_module(const Module* module, void**);
