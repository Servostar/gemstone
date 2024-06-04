
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <llvm/backend.h>
#include <llvm/parser.h>
#include <llvm/types.h>
#include <llvm/variables.h>
#include <llvm/func.h>
#include <set/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/log.h>

BackendError export_IR(LLVMBackendCompileUnit* unit, const Target* target,
                       const TargetConfig* config) {
    DEBUG("exporting module to LLVM-IR...");

    BackendError err = SUCCESS;

    // convert module to LLVM-IR
    char* ir = LLVMPrintModuleToString(unit->module);

    // construct file name
    const char* filename =
        make_file_path(target->name.str, ".ll", 1, config->archive_directory);

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

    free((char*)filename);

    // clean up LLVM-IR string
    LLVMDisposeMessage(ir);

    return err;
}

BackendError emit_module_to_file(LLVMBackendCompileUnit* unit,
                                 LLVMTargetMachineRef target_machine,
                                 LLVMCodeGenFileType file_type, char* error,
                                 const TargetConfig* config) {
    BackendError err = SUCCESS;
    DEBUG("Generating code...");

    const char* filename;
    switch (file_type) {
        case LLVMAssemblyFile:
            filename = make_file_path(config->name, ".s", 1,
                                      config->archive_directory);
            break;
        case LLVMObjectFile:
            filename = make_file_path(config->name, ".o", 1,
                                      config->archive_directory);
            break;
        default:
            return new_backend_impl_error(Implementation, NULL,
                                          "invalid codegen file");
    }

    if (LLVMTargetMachineEmitToFile(target_machine, unit->module, filename,
                                    file_type, &error) != 0) {
        ERROR("failed to emit code: %s", error);
        err =
            new_backend_impl_error(Implementation, NULL, "failed to emit code");
        LLVMDisposeMessage(error);
    }

    free((void*)filename);

    return err;
}

BackendError export_object(LLVMBackendCompileUnit* unit, const Target* target,
                           const TargetConfig* config) {
    BackendError err = SUCCESS;
    DEBUG("exporting object file...");

    INFO("Using target (%s): %s with features: %s", target->name.str,
         target->triple.str, target->features.str);

    LLVMTargetRef llvm_target = NULL;
    char* error = NULL;

    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargetMCs();
    // NOTE: for code generation (assmebly or binary) we need the following:
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    DEBUG("creating target...");
    if (LLVMGetTargetFromTriple(target->triple.str, &llvm_target, &error) !=
        0) {
        ERROR("failed to create target machine: %s", error);
        err = new_backend_impl_error(Implementation, NULL,
                                     "unable to create target machine");
        LLVMDisposeMessage(error);
        return err;
    }

    DEBUG("Creating target machine...");
    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
        llvm_target, target->triple.str, target->cpu.str, target->features.str,
        target->opt, target->reloc, target->model);

    if (config->print_asm) {
        err = emit_module_to_file(unit, target_machine, LLVMAssemblyFile, error,
                                  config);
    }

    if (err.kind != Success) {
        return err;
    }

    err = emit_module_to_file(unit, target_machine, LLVMObjectFile, error,
                              config);

    return err;
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

BackendError export_module(LLVMBackendCompileUnit* unit, const Target* target,
                           const TargetConfig* config) {
    DEBUG("exporting module...");

    BackendError err = SUCCESS;

    export_object(unit, target, config);

    if (config->print_ir) {
        export_IR(unit, target, config);
    }

    return err;
}

static BackendError build_module(LLVMBackendCompileUnit* unit,
                                 LLVMGlobalScope* global_scope,
                                 const Module* module) {
    DEBUG("building module...");
    BackendError err = SUCCESS;

    err = impl_types(unit, global_scope, module->types);
    if (err.kind != Success) {
        return err;
    }

    // NOTE: functions of boxes are not stored in the box itself,
    //       thus for a box we only implement the type
    err = impl_types(unit, global_scope, module->boxes);
    if (err.kind != Success) {
        return err;
    }

    err = impl_global_variables(unit, global_scope, module->variables);
    if (err.kind != Success) {
        return err;
    }

    // TODO: implement functions
    err = impl_functions(unit, global_scope, module->functions);

    return err;
}

BackendError parse_module(const Module* module, const TargetConfig* config) {
    DEBUG("generating code for module %p", module);
    if (module == NULL) {
        ERROR("no module for codegen");
        return new_backend_impl_error(Implementation, NULL, "no module");
    }

    LLVMBackendCompileUnit* unit = malloc(sizeof(LLVMBackendCompileUnit));

    // we start with a LLVM module
    DEBUG("creating LLVM context and module");
    unit->context = LLVMContextCreate();
    unit->module =
        LLVMModuleCreateWithNameInContext(config->name, unit->context);

    LLVMGlobalScope* global_scope = new_global_scope(module);

    DEBUG("generating code...");

    BackendError err = build_module(unit, global_scope, module);
    if (err.kind == Success) {
        INFO("Module build successfully...");
        Target target = create_target_from_config(config);

        export_module(unit, &target, config);

        delete_target(target);
    }

    delete_global_scope(global_scope);

    LLVMDisposeModule(unit->module);
    LLVMContextDispose(unit->context);

    free(unit);

    return err;
}

LLVMGlobalScope* new_global_scope(const Module* module) {
    DEBUG("creating global scope...");
    LLVMGlobalScope* scope = malloc(sizeof(LLVMGlobalScope));

    scope->module = (Module*) module;
    scope->functions = g_hash_table_new(g_str_hash, g_str_equal);
    scope->variables = g_hash_table_new(g_str_hash, g_str_equal);
    scope->types = g_hash_table_new(g_str_hash, g_str_equal);

    return scope;
}

void delete_global_scope(LLVMGlobalScope* scope) {
    DEBUG("deleting global scope...");
    g_hash_table_unref(scope->functions);
    g_hash_table_unref(scope->types);
    g_hash_table_unref(scope->variables);
    free(scope);
}
