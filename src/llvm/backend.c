
#include <assert.h>
#include <ast/ast.h>
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm/backend.h>
#include <llvm/parser.h>
#include <mem/cache.h>
#include <sys/log.h>

Target create_native_target() {
    DEBUG("creating native target...");
    Target target;

    char* triple   = LLVMGetDefaultTargetTriple();
    char* cpu      = LLVMGetHostCPUName();
    char* features = LLVMGetHostCPUFeatures();

    target.name     = "tmp";
    target.triple   = mem_strdup(MemoryNamespaceLld, triple);
    target.cpu      = mem_strdup(MemoryNamespaceLld, cpu);
    target.features = mem_strdup(MemoryNamespaceLld, features);

    LLVMDisposeMessage(triple);
    LLVMDisposeMessage(cpu);
    LLVMDisposeMessage(features);

    target.opt   = LLVMCodeGenLevelNone;
    target.reloc = LLVMRelocDefault;
    target.model = LLVMCodeModelDefault;

    return target;
}

LLVMCodeGenOptLevel llvm_opt_from_int(int level) {
    switch (level) {
        case 1:
            return LLVMCodeGenLevelLess;
        case 2:
            return LLVMCodeGenLevelDefault;
        case 3:
            return LLVMCodeGenLevelAggressive;
        default:
            break;
    }
    PANIC("invalid code generation optimization level: %d", level);
}

static char* create_target_output_name(const TargetConfig* config) {
    char* prefix = "";
    if (config->mode == Library) {
        prefix = "lib";
    }

    char* name        = g_strjoin("", prefix, config->name, NULL);
    char* cached_name = mem_strdup(MemoryNamespaceLlvm, name);
    g_free(name);

    return cached_name;
}

Target create_target_from_triple(char* triple)
{
    Target target;

    target.triple = mem_strdup(MemoryNamespaceLld, triple);

    // select default
    target.cpu = "";

    // select default
    target.features = "";

    return target;
}

Target create_target_from_config(TargetConfig* config) {
    DEBUG("Building target from configuration");

    Target target = create_native_target();
    if (config->triple != NULL)
    {
        target = create_target_from_triple(config->triple);
    } else
    {
        config->triple = target.triple;
    }

    target.name = create_target_output_name(config);

    target.opt = llvm_opt_from_int(config->optimization_level);

    INFO("Configured target: %s/%d: (%s) on %s { %s }", target.name,
         target.opt, target.triple, target.cpu, target.features);

    return target;
}

void delete_target(Target target) {
    mem_free(target.name);
    mem_free(target.cpu);
    mem_free(target.features);
    mem_free(target.triple);
}

typedef enum LLVMBackendError_t { UnresolvedImport } LLVMBackendError;

static BackendError llvm_backend_codegen(const Module* unit,
                                         const TargetConfig* target) {
    return parse_module(unit, target);
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
