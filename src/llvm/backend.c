
#include <codegen/backend.h>
#include <sys/log.h>
#include <ast/ast.h>
#include <llvm/backend.h>
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>

typedef enum LLVMBackendError_t {
    UnresolvedImport
} LLVMBackendError;

static size_t llvm_backend_codegen(const AST_NODE_PTR, void**) {
    // we start with a LLVM module
    LLVMContextRef context = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext("gemstone application", context);

    

    LLVMDisposeModule(module);
    LLVMContextDispose(context);

    return Success;
}

static size_t llvm_backend_codegen_init(void) {
    return Success;
}

static size_t llvm_backend_codegen_deinit(void) {
    return Success;
}

void llvm_backend_init() {
    BackendError err = set_backend(&llvm_backend_codegen_init, &llvm_backend_codegen_deinit, &llvm_backend_codegen, "LLVM");

    if (err != Success) {
        PANIC("unable to init llvm backend: %ld", err);
    }
}
