#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/types.h>
#include <set/types.h>
#include <llvm/parser.h>

BackendError impl_primtive_type(LLVMBackendCompileUnit* unit,
                                PrimitiveType primtive,
                                LLVMTypeRef* llvm_type) {
    switch (primtive) {
        case Int:
            *llvm_type = LLVMInt32TypeInContext(unit->context);
            break;
        case Float:
            *llvm_type = LLVMFloatTypeInContext(unit->context);
            break;
        default:
            break;
    }

    return SUCCESS;
}

BackendError impl_integral_type(LLVMBackendCompileUnit* unit, Scale scale,
                                LLVMTypeRef* llvm_type) {
    LLVMTypeRef integral_type =
        LLVMIntTypeInContext(unit->context, (int)(4 * scale) * 8);

    *llvm_type = integral_type;

    return SUCCESS;
}

BackendError impl_float_type(LLVMBackendCompileUnit* unit, Scale scale,
                             LLVMTypeRef* llvm_type) {
    LLVMTypeRef float_type = NULL;

    switch ((int)(scale * 4)) {
        case 2:
            float_type = LLVMHalfTypeInContext(unit->context);
            break;
        case 4:
            float_type = LLVMFloatTypeInContext(unit->context);
            break;
        case 8:
            float_type = LLVMDoubleTypeInContext(unit->context);
            break;
        case 16:
            float_type = LLVMFP128TypeInContext(unit->context);
            break;
        default:
            break;
    }

    if (float_type != NULL) {
        *llvm_type = float_type;
        return SUCCESS;
    }

    return new_backend_impl_error(Implementation, NULL,
                                  "floating point with scale not supported");
}

BackendError impl_composite_type(LLVMBackendCompileUnit* unit,
                                 CompositeType* composite,
                                 LLVMTypeRef* llvm_type) {
    BackendError err = SUCCESS;

    switch (composite->primitive) {
        case Int:
            err = impl_integral_type(unit, composite->scale, llvm_type);
            break;
        case Float:
            if (composite->sign == Signed) {
                err = impl_float_type(unit, composite->scale, llvm_type);
            } else {
                err = new_backend_impl_error(
                    Implementation, composite->nodePtr,
                    "unsigned floating-point not supported");
            }
            break;
        default:
            break;
    }

    return err;
}

BackendError impl_reference_type(LLVMBackendCompileUnit* unit,
                                 LLVMGlobalScope* scope,
                                 ReferenceType reference,
                                 LLVMTypeRef* llvm_type);

BackendError impl_box_type(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                           BoxType* reference, LLVMTypeRef* llvm_type);

BackendError get_type_impl(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                           Type* gemstone_type, LLVMTypeRef* llvm_type) {
    BackendError err =
        new_backend_impl_error(Implementation, gemstone_type->nodePtr,
                               "No type implementation covers type");

    switch (gemstone_type->kind) {
        case TypeKindPrimitive:
            err = impl_primtive_type(unit, gemstone_type->impl.primitive,
                                     llvm_type);
            break;
        case TypeKindComposite:
            err = impl_composite_type(unit, &gemstone_type->impl.composite,
                                      llvm_type);
            break;
        case TypeKindReference:
            err = impl_reference_type(unit, scope,
                                      gemstone_type->impl.reference, llvm_type);
            break;
        case TypeKindBox:
            err =
                impl_box_type(unit, scope, &gemstone_type->impl.box, llvm_type);
            break;
    }

    return err;
}

BackendError impl_box_type(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                           BoxType* box, LLVMTypeRef* llvm_type) {
    GHashTableIter iterator;
    g_hash_table_iter_init(&iterator, box->member);

    gpointer key = NULL;
    gpointer val = NULL;

    BackendError err;

    GArray* members = g_array_new(FALSE, FALSE, sizeof(LLVMTypeRef));

    while (g_hash_table_iter_next(&iterator, &key, &val) != FALSE) {
        Type* member_type = ((BoxMember*) val)->type;

        LLVMTypeRef llvm_type = NULL;
        err = get_type_impl(unit, scope, member_type, &llvm_type);

        if (err.kind != Success) {
            break;
        }

        g_array_append_val(members, llvm_type);
    }

    if (err.kind == Success) {
        *llvm_type =
            LLVMStructType((LLVMTypeRef*)members->data, members->len, 0);
    }

    g_array_free(members, FALSE);

    return err;
}

BackendError impl_reference_type(LLVMBackendCompileUnit* unit,
                                 LLVMGlobalScope* scope,
                                 ReferenceType reference,
                                 LLVMTypeRef* llvm_type) {
    BackendError err = SUCCESS;
    LLVMTypeRef type = NULL;
    err = get_type_impl(unit, scope, reference, &type);

    if (err.kind == Success) {
        *llvm_type = LLVMPointerType(type, 0);
    }

    return err;
}

BackendError impl_type(LLVMBackendCompileUnit* unit, Type* gemstone_type,
                       const char* alias, LLVMGlobalScope* scope) {
    BackendError err = SUCCESS;

    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope, gemstone_type, &llvm_type);

    if (err.kind == Success) {
        g_hash_table_insert(scope->types, (gpointer)alias, llvm_type);
    }

    return err;
}

BackendError impl_types(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                        GHashTable* types) {
    GHashTableIter iterator;
    g_hash_table_iter_init(&iterator, types);

    gpointer key = NULL;
    gpointer val = NULL;

    BackendError err;

    while (g_hash_table_iter_next(&iterator, &key, &val) != FALSE) {
        err = impl_type(unit, (Type*)val, (const char*)key, scope);

        if (err.kind != Success) {
            break;
        }
    }

    return err;
}

BackendError get_primitive_default_value(PrimitiveType type,
                                         LLVMTypeRef llvm_type,
                                         LLVMValueRef* llvm_value) {
    switch (type) {
        case Int:
            *llvm_value = LLVMConstIntOfString(llvm_type, "0", 10);
            break;
        case Float:
            *llvm_value = LLVMConstRealOfString(llvm_type, "0");
            break;
        default:
            return new_backend_impl_error(Implementation, NULL,
                                          "unknown primitive type");
    }

    return SUCCESS;
}

BackendError get_composite_default_value(CompositeType* composite,
                                         LLVMTypeRef llvm_type,
                                         LLVMValueRef* llvm_value) {
    BackendError err = SUCCESS;

    switch (composite->primitive) {
        case Int:
            err = get_primitive_default_value(Int, llvm_type, llvm_value);
            break;
        case Float:
            if (composite->sign == Signed) {
                err = get_primitive_default_value(Float, llvm_type, llvm_value);
            } else {
                err = new_backend_impl_error(
                    Implementation, composite->nodePtr,
                    "unsigned floating-point not supported");
            }
            break;
        default:
            break;
    }

    return err;
}

BackendError get_reference_default_value(LLVMTypeRef llvm_type,
                                         LLVMValueRef* llvm_value) {
    *llvm_value = LLVMConstPointerNull(llvm_type);

    return SUCCESS;
}

BackendError get_box_default_value(LLVMBackendCompileUnit* unit,
                                   LLVMGlobalScope* scope, BoxType* type,
                                   LLVMValueRef* llvm_value) {
    GHashTableIter iterator;
    g_hash_table_iter_init(&iterator, type->member);

    gpointer key = NULL;
    gpointer val = NULL;

    BackendError err;

    GArray* constants = g_array_new(FALSE, FALSE, sizeof(LLVMValueRef));

    while (g_hash_table_iter_next(&iterator, &key, &val) != FALSE) {
        Type* member_type = ((BoxMember*)val)->type;

        LLVMValueRef constant = NULL;
        err = get_type_default_value(unit, scope, member_type, &constant);

        if (err.kind != Success) {
            break;
        }
    }

    *llvm_value = LLVMConstStructInContext(
        unit->context, (LLVMValueRef*) constants->data, constants->len, 0);

    g_array_free(constants, FALSE);

    return err;
}

BackendError get_type_default_value(LLVMBackendCompileUnit* unit,
                                    LLVMGlobalScope* scope, Type* gemstone_type,
                                    LLVMValueRef* llvm_value) {
    BackendError err = new_backend_impl_error(
        Implementation, gemstone_type->nodePtr, "No default value for type");

    LLVMTypeRef llvm_type = NULL;
    get_type_impl(unit, scope, gemstone_type, &llvm_type);
    if (err.kind != Success) {
        return err;
    }

    switch (gemstone_type->kind) {
        case TypeKindPrimitive:
            err = get_primitive_default_value(gemstone_type->impl.primitive,
                                              llvm_type, llvm_value);
            break;
        case TypeKindComposite:
            err = get_composite_default_value(&gemstone_type->impl.composite,
                                              llvm_type, llvm_value);
            break;
        case TypeKindReference:
            err = get_reference_default_value(llvm_type, llvm_value);
            break;
        case TypeKindBox:
            err = get_box_default_value(unit, scope, &gemstone_type->impl.box,
                                        llvm_value);
            break;
    }

    return err;
}
