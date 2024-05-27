
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <llvm/parser.h>
#include <llvm/types.h>
#include <llvm/variables.h>
#include <set/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "llvm/backend.h"

BackendError export_IR(LLVMBackendCompileUnit* unit) {
    BackendError err = SUCCESS;

    char* ir = LLVMPrintModuleToString(unit->module);

    FILE* output = fopen("module.ll", "w");
    if (output == NULL) {
        err = new_backend_impl_error(Implementation, NULL,
                                     "unable to open file for writing");
        LLVMDisposeMessage(ir);
    }

    fprintf(output, "%s", ir);
    fflush(output);
    fclose(output);

    LLVMDisposeMessage(ir);

    return err;
}

BackendError export_object(LLVMBackendCompileUnit* unit, const Target* target) {
    LLVMTargetRef llvm_target = NULL;
    char* error = NULL;

    if (LLVMGetTargetFromTriple(target->triple.str, &llvm_target, &error) !=
        0) {
        BackendError err =
            new_backend_impl_error(Implementation, NULL, strdup(error));
        LLVMDisposeMessage(error);
        return err;
    }

    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
        llvm_target, target->triple.str, target->cpu.str, target->features.str,
        target->opt, target->reloc, target->model);

    LLVMCodeGenFileType file_type = LLVMObjectFile;

    if (LLVMTargetMachineEmitToFile(target_machine, unit->module, "output",
                                    file_type, &error) != 0) {
        BackendError err =
            new_backend_impl_error(Implementation, NULL, strdup(error));
        LLVMDisposeMessage(error);
        return err;
    }

    return SUCCESS;
}

void list_available_targets() {
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
    BackendError err = SUCCESS;

    export_object(unit, target);

    return err;
}

BackendError parse_module(const Module* module, void**) {
    if (module == NULL) {
        return new_backend_impl_error(Implementation, NULL, "no module");
    }

    LLVMBackendCompileUnit* unit = malloc(sizeof(LLVMBackendCompileUnit));

    // we start with a LLVM module
    unit->context = LLVMContextCreate();
    unit->module = LLVMModuleCreateWithNameInContext("gemstone application",
                                                     unit->context);

    LLVMGlobalScope* global_scope = new_global_scope();

    BackendError err = new_backend_error(Success);

    err = impl_types(unit, global_scope, module->types);
    // NOTE: functions of boxes are not stored in the box itself,
    //       thus for a box we only implement the type
    err = impl_types(unit, global_scope, module->boxes);

    err = impl_global_variables(unit, global_scope, module->variables);

    char* err_msg = NULL;
    if (LLVMPrintModuleToFile(unit->module, "out.s", &err_msg)) {
        err = new_backend_impl_error(Implementation, NULL, err_msg);
    }

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
    LLVMGlobalScope* scope = malloc(sizeof(LLVMGlobalScope));

    scope->functions = g_hash_table_new(g_str_hash, g_str_equal);
    scope->variables = g_hash_table_new(g_str_hash, g_str_equal);
    scope->types = g_hash_table_new(g_str_hash, g_str_equal);

    return scope;
}

LLVMLocalScope* new_local_scope(LLVMGlobalScope* global_scope,
                                LLVMLocalScope* parent_scope) {
    LLVMLocalScope* scope = malloc(sizeof(LLVMLocalScope));

    scope->variables = g_hash_table_new(g_str_hash, g_str_equal);
    scope->params = g_hash_table_new(g_str_hash, g_str_equal);
    scope->global_scope = global_scope;
    scope->parent_scope = parent_scope;

    return scope;
}

void delete_local_scope(LLVMLocalScope* scope) {
    g_hash_table_unref(scope->variables);
    free(scope);
}

void delete_global_scope(LLVMGlobalScope* scope) {
    g_hash_table_unref(scope->functions);
    g_hash_table_unref(scope->types);
    g_hash_table_unref(scope->variables);
    free(scope);
}
