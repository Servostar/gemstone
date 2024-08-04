
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
#include <mem/cache.h>
#include <llvm/llvm-ir/expr.h>

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

LLVMValueRef get_parameter(const LLVMFuncScope* scope,
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

    LLVMValueRef global_var = get_global_variable(scope->func_scope->global_scope, (char*) name);
    return global_var;
}

LLVMBool is_parameter(const LLVMLocalScope* scope, const char* name) {
    if (g_hash_table_contains(scope->vars, name)) {
        return FALSE;
    }

    if (scope->parent_scope != NULL) {
        return is_parameter(scope->parent_scope, name);
    }

    LLVMValueRef param = get_parameter(scope->func_scope, name);
    if (param != NULL) {
        return TRUE;
    }

    LLVMValueRef global_var = get_global_variable(scope->func_scope->global_scope, (char*) name);
    return global_var != NULL;
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

BackendError impl_func_type(LLVMBackendCompileUnit* unit,
                            LLVMGlobalScope* scope, Function* func,
                            LLVMValueRef* llvm_fun) {
    DEBUG("implementing function declaration: %s()", func->name);
    BackendError err = SUCCESS;

    GArray* llvm_params = mem_new_g_array(MemoryNamespaceLlvm, sizeof(LLVMTypeRef));
    GArray* func_params = NULL;

    if (func->kind == FunctionDeclarationKind) {
        func_params = func->impl.declaration.parameter;
    } else {
        func_params = func->impl.definition.parameter;
    }

    for (size_t i = 0; i < func_params->len; i++) {
        Parameter* param = &g_array_index(func_params, Parameter, i);

        LLVMTypeRef llvm_type = NULL;
        err = impl_param_type(unit, scope, param, &llvm_type);

        if (err.kind != Success) {
            return err;
        }

        g_array_append_val(llvm_params, llvm_type);
    }

    DEBUG("implemented %ld parameter", llvm_params->len);

    LLVMTypeRef llvm_return_type = LLVMVoidTypeInContext(unit->context);
    if (func->kind == FunctionDeclarationKind) {
        if (func->impl.declaration.return_value != NULL) {
            err = get_type_impl(unit, scope, func->impl.declaration.return_value, &llvm_return_type);
        }
    } else {
        if (func->impl.definition.return_value != NULL) {
            err = get_type_impl(unit, scope, func->impl.definition.return_value, &llvm_return_type);
        }
    }

    LLVMTypeRef llvm_fun_type =
        LLVMFunctionType(llvm_return_type,
                         (LLVMTypeRef*)llvm_params->data, llvm_params->len, 0);

    *llvm_fun = LLVMAddFunction(unit->module, func->name, llvm_fun_type);

    g_hash_table_insert(scope->functions, (char*) func->name, llvm_fun_type);

    return err;
}

BackendError impl_func_def(LLVMBackendCompileUnit* unit,
                       LLVMGlobalScope* global_scope,
                       Function* func, const char* name) {
    BackendError err = SUCCESS;

    LLVMValueRef llvm_func = LLVMGetNamedFunction(unit->module, name);

    if (err.kind == Success) {
        // create local function scope
        // NOTE: lives till the end of the function
        LLVMFuncScope* func_scope = alloca(sizeof(LLVMFuncScope));

        func_scope->llvm_func = llvm_func;
        func_scope->global_scope = global_scope;
        func_scope->params = g_hash_table_new(g_str_hash, g_str_equal);

        // create function body builder
        LLVMBasicBlockRef entry =
            LLVMAppendBasicBlockInContext(unit->context, llvm_func, "func.entry");
        LLVMBuilderRef builder = LLVMCreateBuilderInContext(unit->context);
        LLVMPositionBuilderAtEnd(builder, entry);

        // create value references for parameter
        for (guint i = 0; i < func->impl.definition.parameter->len; i++) {
            Parameter* param = &g_array_index(func->impl.definition.parameter, Parameter, i);
            LLVMValueRef llvm_param = LLVMGetParam(llvm_func, i);

            if (llvm_param == NULL) {
                return new_backend_impl_error(Implementation, NULL, "invalid parameter");
            }

            g_hash_table_insert(func_scope->params, (gpointer)param->name, llvm_param);
        }

        LLVMBasicBlockRef llvm_start_body_block = NULL;
        LLVMBasicBlockRef llvm_end_body_block = NULL;
        err = impl_block(unit, builder, func_scope, &llvm_start_body_block, &llvm_end_body_block, func->impl.definition.body);

        if (err.kind == Success) {
            LLVMPositionBuilderAtEnd(builder, entry);
            LLVMBuildBr(builder, llvm_start_body_block);

            LLVMValueRef terminator = LLVMGetBasicBlockTerminator(llvm_end_body_block);
            if (terminator == NULL) {
                // insert returning end block
                LLVMBasicBlockRef end_block =
                        LLVMAppendBasicBlockInContext(unit->context, llvm_func, "func.end");
                LLVMPositionBuilderAtEnd(builder, end_block);

                LLVMValueRef llvm_return = NULL;
                if (func->kind == FunctionDeclarationKind) {
                    if (func->impl.declaration.return_value != NULL) {
                        err = get_type_default_value(unit, global_scope, func->impl.declaration.return_value, &llvm_return);
                        if (err.kind != Success) {
                            return err;
                        }
                        LLVMBuildRet(builder, llvm_return);
                    }else {
                        LLVMBuildRetVoid(builder);
                    }
                } else {
                    if (func->impl.definition.return_value != NULL) {
                        err = get_type_default_value(unit, global_scope, func->impl.definition.return_value, &llvm_return);
                        if (err.kind != Success) {
                            return err;
                        }
                        LLVMBuildRet(builder, llvm_return);
                    } else {
                        LLVMBuildRetVoid(builder);
                    }
                }

                LLVMPositionBuilderAtEnd(builder, llvm_end_body_block);
                LLVMBuildBr(builder, end_block);
            }

            LLVMDisposeBuilder(builder);
        }

        // delete function scope GLib structs
        g_hash_table_destroy(func_scope->params);
    }

    return err;
}

BackendError impl_function_types(LLVMBackendCompileUnit* unit,
                            LLVMGlobalScope* scope, GHashTable* functions) {
    DEBUG("implementing functions...");
    GHashTableIter iterator;
    g_hash_table_iter_init(&iterator, functions);

    gpointer key = NULL;
    gpointer val = NULL;

    BackendError err = SUCCESS;
    while (g_hash_table_iter_next(&iterator, &key, &val) != FALSE) {
        Function* func = (Function*) val;
        LLVMValueRef llvm_func;
        err = impl_func_type(unit, scope, func, &llvm_func);
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

        if (func->kind != FunctionDeclarationKind) {
            err = impl_func_def(unit, scope, func, (const char*)key);
        }

        if (err.kind != Success) {
            return err;
        }

        function_count++;
    }
    INFO("implemented %ld functions", function_count);

    return err;
}

gboolean is_parameter_out(Parameter *param) {
    gboolean is_out = FALSE;

    if (param->kind == ParameterDeclarationKind) {
        is_out = param->impl.declaration.qualifier == Out || param->impl.declaration.qualifier == InOut;
    } else {
        is_out = param->impl.definiton.declaration.qualifier == Out ||
                 param->impl.definiton.declaration.qualifier == InOut;
    }

    return is_out;
}

BackendError impl_func_call(LLVMBackendCompileUnit *unit,
                            LLVMBuilderRef builder, LLVMLocalScope *scope,
                            const FunctionCall *call,
                            LLVMValueRef* return_value) {
    DEBUG("implementing function call...");
    BackendError err = SUCCESS;

    LLVMValueRef* arguments = NULL;

    // prevent memory allocation when number of bytes would be zero
    // avoid going of assertion in memory cache
    if (call->expressions->len > 0) {
        arguments = mem_alloc(MemoryNamespaceLlvm, sizeof(LLVMValueRef) * call->expressions->len);

        for (size_t i = 0; i < call->expressions->len; i++) {
            Expression *arg = g_array_index(call->expressions, Expression*, i);

            GArray* param_list;
            if (call->function->kind == FunctionDeclarationKind) {
                param_list = call->function->impl.definition.parameter;
            } else {
                param_list = call->function->impl.declaration.parameter;
            }

            Parameter param = g_array_index(param_list, Parameter, i);

            LLVMValueRef llvm_arg = NULL;
            err = impl_expr(unit, scope, builder, arg, is_parameter_out(&param), 0, &llvm_arg);

            if (err.kind != Success) {
                break;
            }

            if (is_parameter_out(&param)) {
                if ((arg->kind == ExpressionKindParameter && !is_parameter_out(arg->impl.parameter)) || arg->kind != ExpressionKindParameter) {
                    LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), 0, false);
                    LLVMTypeRef llvm_type = NULL;
                    get_type_impl(unit, scope->func_scope->global_scope, param.impl.declaration.type, &llvm_type);
                    llvm_arg = LLVMBuildGEP2(builder, llvm_type, llvm_arg, &index, 1, "");
                }
            }

            arguments[i] = llvm_arg;
        }
    }

    if (err.kind == Success) {
        LLVMValueRef llvm_func = LLVMGetNamedFunction(unit->module, call->function->name);

        if (llvm_func == NULL) {
            return new_backend_impl_error(Implementation, NULL, "no declared function");
        }

        LLVMTypeRef llvm_func_type = g_hash_table_lookup(scope->func_scope->global_scope->functions, call->function->name);

        LLVMValueRef value = LLVMBuildCall2(builder, llvm_func_type, llvm_func, arguments, call->expressions->len,
                   "");

        if (NULL != return_value) {
            *return_value = value;
        }
    }

    return err;
}

