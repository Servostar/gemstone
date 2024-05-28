
#include <alloca.h>
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/parser.h>
#include <llvm/types.h>
#include <set/types.h>
#include <sys/log.h>
#include <llvm/func.h>

static LLVMValueRef get_parameter(const LLVMFuncScope* scope, const char* name) {
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

    return get_parameter(scope->func_scope, name);
}

BackendError impl_param_type(LLVMBackendCompileUnit* unit,
                             LLVMGlobalScope* scope, Paramer* param,
                             LLVMTypeRef* llvm_type) {
    BackendError err = SUCCESS;

    Type* gemstone_type = NULL;
    IO_Qualifier qualifier = In;

    if (param->kind == ParameterDeclarationKind) {
        gemstone_type = &param->impl.declaration.type;
        qualifier = param->impl.declaration.qualifier;
    } else {
        gemstone_type = &param->impl.definiton.declaration.type;
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

    Paramer* params = (Paramer*)fundef->parameter;
    GArray* llvm_params = g_array_new(FALSE, FALSE, sizeof(LLVMTypeRef));

    for (size_t i = 0; i < fundef->parameter->len; i++) {
        Paramer* param = &params[i];

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

BackendError impl_func(LLVMBackendCompileUnit* unit, LLVMGlobalScope* global_scope,
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
        g_hash_table_insert(global_scope->functions, (gpointer) name, llvm_func);

        // create function body builder
        LLVMBasicBlockRef body = LLVMAppendBasicBlockInContext(unit->context, llvm_func, "entry");
        LLVMBuilderRef builder = LLVMCreateBuilderInContext(unit->context);
        LLVMPositionBuilderAtEnd(builder, body);

        // create value references for parameter
        const size_t params = fundef->parameter->len;
        for (size_t i = 0; i < params; i++) {
            const Paramer* param = ((Paramer*) fundef->parameter) + i;
            g_hash_table_insert(func_scope->params, (gpointer) param->name, LLVMGetParam(llvm_func, i));
        }

        // parse function body

        // delete function scope GLib structs
        g_hash_table_destroy(func_scope->params);
    }

    return err;
}
