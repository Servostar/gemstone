
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/parser.h>
#include <llvm/types.h>
#include <llvm/variables.h>
#include <set/types.h>
#include <stdlib.h>

BackendError parse_module(const Module* module, void**) {
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
