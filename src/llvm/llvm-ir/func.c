
#include <alloca.h>
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/llvm-ir/func.h>
#include <llvm/parser.h>
#include <llvm/llvm-ir/types.h>
#include <llvm/llvm-ir/stmt.h>
#include <llvm/llvm-ir/variables.h>
#include <set/types.h>
#include <sys/log.h>

LLVMLocalScope* new_local_scope(LLVMLocalScope* parent) {
    LLVMLocalScope* scope = malloc(sizeof(LLVMLocalScope));

    scope->func_scope = parent->func_scope;
    scope->vars = g_hash_table_new(g_str_hash, g_str_equal);
    scope->parent_scope = parent;

    return scope;
}

void delete_local_scope(LLVMLocalScope* scope) {
    g_hash_table_destroy(scope->vars);
    free(scope);
}

static LLVMValueRef get_parameter(const LLVMFuncScope* scope,
                                  const char* name) {
    if (g_hash_table_contains(scope->params, name)) {
        return g_hash_table_lookup(scope->params, name);
    }

    return NULL;
}

LLVMValueRef get_variable(const LLVMLocalScope* scope, const char* name) {
    if (g_hash_table_contains(scope->vars, name)) {
        return g_hash_table_lookup(scope->vars, name);
    }

    if (scope->parent_scope != NULL) {
        return get_variable(scope->parent_scope, name);
    }

    LLVMValueRef param = get_parameter(scope->func_scope, name);
    if (param != NULL) {
        return param;
    }

    LLVMValueRef global_var = get_global_variable(scope->func_scope->global_scope, name);
    return global_var;
}

BackendError impl_param_type(LLVMBackendCompileUnit* unit,
                             LLVMGlobalScope* scope, Parameter* param,
                             LLVMTypeRef* llvm_type) {
    BackendError err = SUCCESS;

    Type* gemstone_type = NULL;
    IO_Qualifier qualifier;

    if (param->kind == ParameterDeclarationKind) {
        gemstone_type = param->impl.declaration.type;
        qualifier = param->impl.declaration.qualifier;
    } else {
        gemstone_type = param->impl.definiton.declaration.type;
        qualifier = param->impl.definiton.declaration.qualifier;
    }

    // wrap output variables as pointers
    if (qualifier == Out || qualifier == InOut) {
        Type* reference_type = alloca(sizeof(Type));

        reference_type->kind = TypeKindReference;
        reference_type->impl.reference = gemstone_type;

        gemstone_type = reference_type;
    }

    err = get_type_impl(unit, scope, gemstone_type, llvm_type);

    return err;
}

BackendError impl_func_decl(LLVMBackendCompileUnit* unit,
                            LLVMGlobalScope* scope, FunctionDefinition* fundef,
                            LLVMValueRef* llvm_fun, const char* name) {
    DEBUG("implementing function declaration: %s()", name);
    BackendError err = SUCCESS;

    Parameter* params = (Parameter*)fundef->parameter;
    GArray* llvm_params = g_array_new(FALSE, FALSE, sizeof(LLVMTypeRef));

    for (size_t i = 0; i < fundef->parameter->len; i++) {
        Parameter* param = &params[i];

        LLVMTypeRef llvm_type = NULL;
        err = impl_param_type(unit, scope, param, &llvm_type);

        if (err.kind != Success) {
            break;
        }
    }

    DEBUG("implemented %ld parameter", llvm_params->len);

    LLVMTypeRef llvm_fun_type =
        LLVMFunctionType(LLVMVoidTypeInContext(unit->context),
                         (LLVMTypeRef*)llvm_params->data, llvm_params->len, 0);

    *llvm_fun = LLVMAddFunction(unit->module, name, llvm_fun_type);

    g_array_free(llvm_params, FALSE);

    return err;
}

BackendError impl_func_def(LLVMBackendCompileUnit* unit,
                       LLVMGlobalScope* global_scope,
                       FunctionDefinition* fundef, const char* name) {
    BackendError err = SUCCESS;

    LLVMValueRef llvm_func = NULL;
    err = impl_func_decl(unit, global_scope, fundef, &llvm_func, name);

    if (err.kind == Success) {
        // create local function scope
        // NOTE: lives till the end of the function
        LLVMFuncScope* func_scope = alloca(sizeof(LLVMFuncScope));

        func_scope->llvm_func = llvm_func;
        func_scope->global_scope = global_scope;
        func_scope->params = g_hash_table_new(g_str_hash, g_str_equal);

        // store function type in global scope
        g_hash_table_insert(global_scope->functions, (gpointer)name, llvm_func);

        // create function body builder
        LLVMBasicBlockRef entry =
            LLVMAppendBasicBlockInContext(unit->context, llvm_func, "func.entry");
        LLVMBuilderRef builder = LLVMCreateBuilderInContext(unit->context);
        LLVMPositionBuilderAtEnd(builder, entry);

        // create value references for parameter
        const size_t params = fundef->parameter->len;
        for (size_t i = 0; i < params; i++) {
            const Parameter* param = ((Parameter*)fundef->parameter) + i;
            g_hash_table_insert(func_scope->params, (gpointer)param->name,
                                LLVMGetParam(llvm_func, i));
        }

        LLVMBasicBlockRef llvm_start_body_block = NULL;
        LLVMBasicBlockRef llvm_end_body_block = NULL;
        err = impl_block(unit, builder, func_scope, &llvm_start_body_block, &llvm_end_body_block, fundef->body);
        LLVMPositionBuilderAtEnd(builder, entry);
        LLVMBuildBr(builder, llvm_start_body_block);

        // insert returning end block
        LLVMBasicBlockRef end_block =
                LLVMAppendBasicBlockInContext(unit->context, llvm_func, "func.end");
        LLVMPositionBuilderAtEnd(builder, end_block);
        LLVMBuildRetVoid(builder);

        LLVMPositionBuilderAtEnd(builder, llvm_end_body_block);
        LLVMBuildBr(builder, end_block);

        LLVMDisposeBuilder(builder);

        // delete function scope GLib structs
        g_hash_table_destroy(func_scope->params);
    }

    return err;
}

BackendError impl_functions(LLVMBackendCompileUnit* unit,
                            LLVMGlobalScope* scope, GHashTable* functions) {
    DEBUG("implementing functions...");
    GHashTableIter iterator;
    g_hash_table_iter_init(&iterator, functions);

    gpointer key = NULL;
    gpointer val = NULL;

    BackendError err = SUCCESS;

    size_t function_count = 0;
    while (g_hash_table_iter_next(&iterator, &key, &val) != FALSE) {
        Function* func = (Function*) val;

        if (func->kind == FunctionDeclarationKind) {
            // err = impl_func_decl(unit, scope, &func->impl.declaration, (const char*) key);
        } else {
            err = impl_func_def(unit, scope, &func->impl.definition, (const char*)key);
        }

        if (err.kind != Success) {
            return err;
        }

        function_count++;
    }
    INFO("implemented %ld functions", function_count);

    return err;
}
