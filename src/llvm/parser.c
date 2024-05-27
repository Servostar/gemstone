
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <llvm/backend.h>
#include <llvm/parser.h>
#include <llvm/types.h>
#include <llvm/variables.h>
#include <set/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/log.h>

#define EXPORT_IR 1

BackendError export_IR(LLVMBackendCompileUnit* unit, const Target* target) {
    DEBUG("exporting module to LLVM-IR...");

    BackendError err = SUCCESS;

    // convert module to LLVM-IR
    char* ir = LLVMPrintModuleToString(unit->module);

    // construct file name
    char* filename = alloca(strlen(target->name.str) + 4);
    sprintf(filename, "%s.ll", target->name.str);

    INFO("Writing LLVM-IR to %s", filename);

    DEBUG("opening file...");
    FILE* output = fopen(filename, "w");
    if (output == NULL) {
        ERROR("unable to open file: %s", filename);
        err = new_backend_impl_error(Implementation, NULL,
                                     "unable to open file for writing");
        LLVMDisposeMessage(ir);
    }

    DEBUG("printing LLVM-IR to file...");

    size_t bytes = fprintf(output, "%s", ir);

    // flush and close output file
    fflush(output);
    fclose(output);

    INFO("%ld bytes written to %s", bytes, filename);

    // clean up LLVM-IR string
    LLVMDisposeMessage(ir);

    return err;
}

BackendError export_object(LLVMBackendCompileUnit* unit, const Target* target) {
    DEBUG("exporting object file...");

    INFO("Using target (%s): %s with features: %s", target->name.str,
         target->triple.str, target->features.str);

    LLVMTargetRef llvm_target = NULL;
    char* error = NULL;

    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();

    DEBUG("creating target...");
    if (LLVMGetTargetFromTriple(target->triple.str, &llvm_target, &error) !=
        0) {
        ERROR("failed to create target machine: %s", error);
        BackendError err = new_backend_impl_error(
            Implementation, NULL, "unable to create target machine");
        LLVMDisposeMessage(error);
        return err;
    }

    DEBUG("Creating target machine...");
    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
        llvm_target, target->triple.str, target->cpu.str, target->features.str,
        target->opt, target->reloc, target->model);

    LLVMCodeGenFileType file_type = LLVMObjectFile;

    DEBUG("Generating code...");
    if (LLVMTargetMachineEmitToFile(target_machine, unit->module, "output",
                                    file_type, &error) != 0) {
        ERROR("failed to emit code: %s", error);
        BackendError err =
            new_backend_impl_error(Implementation, NULL, "failed to emit code");
        LLVMDisposeMessage(error);
        return err;
    }

    return SUCCESS;
}

void list_available_targets() {
    DEBUG("initializing all available targets...");
    LLVMInitializeAllTargetInfos();

    printf("Available targets:\n");

    LLVMTargetRef target = LLVMGetFirstTarget();
    while (target) {
        const char* name = LLVMGetTargetName(target);
        const char* desc = LLVMGetTargetDescription(target);
        printf(" - %s: (%s)\n", name, desc);

        target = LLVMGetNextTarget(target);
    }

    char* default_triple = LLVMGetDefaultTargetTriple();

    printf("Default: %s\n", default_triple);

    LLVMDisposeMessage(default_triple);
}

BackendError export_module(LLVMBackendCompileUnit* unit, const Target* target) {
    DEBUG("exporting module...");

    BackendError err = SUCCESS;

    export_object(unit, target);

    if (EXPORT_IR) {
        export_IR(unit, target);
    } else {
        DEBUG("not exporting LLVM-IR");
    }

    return err;
}

BackendError parse_module(const Module* module, void**) {
    DEBUG("generating code for module %p", module);
    if (module == NULL) {
        ERROR("no module for codegen");
        return new_backend_impl_error(Implementation, NULL, "no module");
    }

    LLVMBackendCompileUnit* unit = malloc(sizeof(LLVMBackendCompileUnit));

    // we start with a LLVM module
    DEBUG("creating LLVM context and module");
    unit->context = LLVMContextCreate();
    unit->module = LLVMModuleCreateWithNameInContext("gemstone application",
                                                     unit->context);

    LLVMGlobalScope* global_scope = new_global_scope();

    BackendError err = new_backend_error(Success);

    DEBUG("generating code...");

    err = impl_types(unit, global_scope, module->types);
    // NOTE: functions of boxes are not stored in the box itself,
    //       thus for a box we only implement the type
    err = impl_types(unit, global_scope, module->boxes);

    err = impl_global_variables(unit, global_scope, module->variables);

    Target target = create_native_target();

    export_module(unit, &target);

    delete_target(target);

    delete_global_scope(global_scope);

    LLVMDisposeModule(unit->module);
    LLVMContextDispose(unit->context);

    free(unit);

    return err;
}

LLVMGlobalScope* new_global_scope() {
    DEBUG("creating global scope...");
    LLVMGlobalScope* scope = malloc(sizeof(LLVMGlobalScope));

    scope->functions = g_hash_table_new(g_str_hash, g_str_equal);
    scope->variables = g_hash_table_new(g_str_hash, g_str_equal);
    scope->types = g_hash_table_new(g_str_hash, g_str_equal);

    return scope;
}

LLVMLocalScope* new_local_scope(LLVMGlobalScope* global_scope,
                                LLVMLocalScope* parent_scope) {
    DEBUG("creating local scope...");
    LLVMLocalScope* scope = malloc(sizeof(LLVMLocalScope));

    scope->variables = g_hash_table_new(g_str_hash, g_str_equal);
    scope->params = g_hash_table_new(g_str_hash, g_str_equal);
    scope->global_scope = global_scope;
    scope->parent_scope = parent_scope;

    return scope;
}

void delete_local_scope(LLVMLocalScope* scope) {
    DEBUG("deleting global scope...");
    g_hash_table_unref(scope->variables);
    free(scope);
}

void delete_global_scope(LLVMGlobalScope* scope) {
    DEBUG("deleting global scope...");
    g_hash_table_unref(scope->functions);
    g_hash_table_unref(scope->types);
    g_hash_table_unref(scope->variables);
    free(scope);
}
