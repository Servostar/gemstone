
#include <alloca.h>
#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/parser.h>
#include <llvm/types.h>
#include <set/types.h>
#include <sys/log.h>

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

BackendError impl_func(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                       FunctionDefinition* fundef, const char* name) {
    BackendError err = SUCCESS;


    return err;
}
