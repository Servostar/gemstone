
#include <assert.h>
#include <ast/ast.h>
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm/backend.h>
#include <llvm/parser.h>
#include <sys/log.h>

Target create_native_target() {
    DEBUG("creating native target...");
    Target target;

    target.name.str = "tmp";
    target.name.allocation = NONE;

    target.triple.str = LLVMGetDefaultTargetTriple();
    target.triple.allocation = LLVM;
    assert(target.triple.str != NULL);

    target.cpu.str = LLVMGetHostCPUName();
    target.cpu.allocation = LLVM;
    assert(target.cpu.str != NULL);

    target.features.str = LLVMGetHostCPUFeatures();
    target.features.allocation = LLVM;
    assert(target.features.str != NULL);

    target.opt = LLVMCodeGenLevelDefault;
    target.reloc = LLVMRelocDefault;
    target.model = LLVMCodeModelDefault;

    return target;
}

Target create_target_from_config() { PANIC("NOT IMPLEMENTED"); }

static void delete_string(String string) {
    DEBUG("deleting string...");
    switch (string.allocation) {
        case LLVM:
            LLVMDisposeMessage(string.str);
            break;
        case LIBC:
            free(string.str);
            break;
        case NONE:
            break;
    }
}

void delete_target(Target target) {
    delete_string(target.name);
    delete_string(target.cpu);
    delete_string(target.features);
    delete_string(target.triple);
}

typedef enum LLVMBackendError_t { UnresolvedImport } LLVMBackendError;

static BackendError llvm_backend_codegen(const Module* unit, void** output) {
    return parse_module(unit, output);
}

static BackendError llvm_backend_codegen_init(void) {
    return new_backend_error(Success);
}

static BackendError llvm_backend_codegen_deinit(void) {
    return new_backend_error(Success);
}

void llvm_backend_init() {
    BackendError err =
        set_backend(&llvm_backend_codegen_init, &llvm_backend_codegen_deinit,
                    &llvm_backend_codegen, "LLVM");

    if (err.kind != Success) {
        PANIC("unable to init llvm backend: %ld", err);
    }
}
