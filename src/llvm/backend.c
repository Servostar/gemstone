
#include <codegen/backend.h>
#include <sys/log.h>
#include <ast/ast.h>
#include <llvm/backend.h>
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>

typedef enum LLVMBackendError_t {
    UnresolvedImport
} LLVMBackendError;

static BackendError llvm_backend_codegen(const Module* unit, void**) {
    // we start with a LLVM module
    LLVMContextRef context = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext("gemstone application", context);

    BackendError err;

    LLVMDisposeModule(module);
    LLVMContextDispose(context);

    return new_backend_error(Success);
}

static BackendError llvm_backend_codegen_init(void) {
    return new_backend_error(Success);
}

static BackendError llvm_backend_codegen_deinit(void) {
    return new_backend_error(Success);
}

void llvm_backend_init() {
    BackendError err = set_backend(&llvm_backend_codegen_init, &llvm_backend_codegen_deinit, &llvm_backend_codegen, "LLVM");

    if (err.kind != Success) {
        PANIC("unable to init llvm backend: %ld", err);
    }
}
