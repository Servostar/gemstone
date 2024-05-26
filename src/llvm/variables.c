
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/types.h>
#include <llvm/variables.h>
#include <set/types.h>

BackendError impl_global_declaration(LLVMBackendCompileUnit* unit,
                                     LLVMGlobalScope* scope,
                                     VariableDeclaration* decl,
                                     const char* name) {
    BackendError err = SUCCESS;
    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope, &decl->type, &llvm_type);

    if (err.kind != Success) {
        return err;
    }

    LLVMValueRef global = LLVMAddGlobal(unit->module, llvm_type, name);

    LLVMValueRef initial_value = NULL;
    err = get_type_default_value(unit, scope, &decl->type, &initial_value);

    if (err.kind == Success) {
        LLVMSetInitializer(global, initial_value);
        g_hash_table_insert(scope->variables, (gpointer)name, global);
    }

    return err;
}

BackendError impl_global_definiton(LLVMBackendCompileUnit* unit,
                                     LLVMGlobalScope* scope,
                                     VariableDefiniton* def,
                                     const char* name) {
    BackendError err = SUCCESS;
    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope, &def->declaration.type, &llvm_type);

    if (err.kind != Success) {
        return err;
    }

    LLVMValueRef global = LLVMAddGlobal(unit->module, llvm_type, name);

    // FIXME: resolve initializer expression!
    LLVMValueRef initial_value = NULL;
    err = get_type_default_value(unit, scope, &def->declaration.type, &initial_value);

    if (err.kind == Success) {
        LLVMSetInitializer(global, initial_value);
        g_hash_table_insert(scope->variables, (gpointer)name, global);
    }

    return err;
}

BackendError impl_global_variable(LLVMBackendCompileUnit* unit,
                                  Variable* gemstone_var, const char* alias,
                                  LLVMGlobalScope* scope) {
    BackendError err = SUCCESS;

    switch (gemstone_var->kind) {
        case VariableKindDeclaration:
            err = impl_global_declaration(
                unit, scope, &gemstone_var->impl.declaration, alias);
            break;
        case VariableKindDefinition:
            err = impl_global_definiton(
                unit, scope, &gemstone_var->impl.definiton, alias);
            break;
        case VariableKindBoxMember:
            err = new_backend_impl_error(Implementation, gemstone_var->nodePtr,
                                         "member variable cannot be ");
            break;
    }

    return err;
}

BackendError impl_global_variables(LLVMBackendCompileUnit* unit,
                                   LLVMGlobalScope* scope,
                                   GHashTable* variables) {
    GHashTableIter iterator;
    g_hash_table_iter_init(&iterator, variables);

    gpointer key = NULL;
    gpointer val = NULL;

    BackendError err;

    while (g_hash_table_iter_next(&iterator, &key, &val) != FALSE) {
        err =
            impl_global_variable(unit, (Variable*)val, (const char*)key, scope);

        if (err.kind != Success) {
            break;
        }
    }

    return err;
}
