#include <codegen/backend.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/llvm-ir/types.h>
#include <llvm/parser.h>
#include <set/types.h>
#include <sys/log.h>

#define BASE_BYTES 4
#define BITS_PER_BYTE 8

static BackendError get_const_primitive_value(PrimitiveType primitive,
                                              LLVMTypeRef llvm_type,
                                              const char* value,
                                              LLVMValueRef* llvm_value) {
    switch (primitive) {
        case Int:
            *llvm_value = LLVMConstIntOfString(llvm_type, value, 10);
            break;
        case Float:
            *llvm_value = LLVMConstRealOfString(llvm_type, value);
            break;
    }

    return SUCCESS;
}

static BackendError get_const_composite_value(CompositeType composite,
                                              LLVMTypeRef llvm_type,
                                              const char* value,
                                              LLVMValueRef* llvm_value) {
    return get_const_primitive_value(composite.primitive, llvm_type, value,
                                     llvm_value);
}

BackendError get_const_type_value(LLVMBackendCompileUnit* unit,
                                  LLVMGlobalScope* scope,
                                  TypeValue* gemstone_value,
                                  LLVMValueRef* llvm_value) {
    BackendError err = new_backend_impl_error(
        Implementation, gemstone_value->nodePtr, "No default value for type");

    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope, &gemstone_value->type, &llvm_type);
    if (err.kind != Success) {
        return err;
    }

    switch (gemstone_value->type.kind) {
        case TypeKindPrimitive:
            err = get_const_primitive_value(gemstone_value->type.impl.primitive,
                                            llvm_type, gemstone_value->value,
                                            llvm_value);
            break;
        case TypeKindComposite:
            err = get_const_composite_value(gemstone_value->type.impl.composite,
                                            llvm_type, gemstone_value->value,
                                            llvm_value);
            break;
        case TypeKindReference:
            err =
                new_backend_impl_error(Implementation, gemstone_value->nodePtr,
                                       "reference cannot be constant value");
            break;
        case TypeKindBox:
            err =
                new_backend_impl_error(Implementation, gemstone_value->nodePtr,
                                       "boxes cannot be constant value");
            break;
        default:
            PANIC("invalid value kind: %ld", gemstone_value->type.kind);
    }

    return err;
}

BackendError impl_primtive_type(LLVMBackendCompileUnit* unit,
                                PrimitiveType primtive,
                                LLVMTypeRef* llvm_type) {
    switch (primtive) {
        case Int:
            DEBUG("implementing primtive integral type...");
            *llvm_type = LLVMInt32TypeInContext(unit->context);
            break;
        case Float:
            DEBUG("implementing primtive float type...");
            *llvm_type = LLVMFloatTypeInContext(unit->context);
            break;
        default:
            PANIC("invalid primitive type");
            break;
    }

    return SUCCESS;
}

BackendError impl_integral_type(LLVMBackendCompileUnit* unit, Scale scale,
                                LLVMTypeRef* llvm_type) {
    size_t bits = (int)(4 * scale) * BITS_PER_BYTE;
    DEBUG("implementing integral type of size: %ld", bits);
    LLVMTypeRef integral_type = LLVMIntTypeInContext(unit->context, bits);

    *llvm_type = integral_type;

    return SUCCESS;
}

BackendError impl_float_type(LLVMBackendCompileUnit* unit, Scale scale,
                             LLVMTypeRef* llvm_type) {
    DEBUG("implementing floating point...");
    LLVMTypeRef float_type = NULL;

    size_t bytes = (int)(scale * BASE_BYTES);
    DEBUG("requested float of bytes: %ld", bytes);
    switch (bytes) {
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
            ERROR("invalid floating point size: %ld bit",
                  bytes * BITS_PER_BYTE);
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
    DEBUG("implementing composite type...");
    BackendError err = SUCCESS;

    switch (composite->primitive) {
        case Int:
            err = impl_integral_type(unit, composite->scale, llvm_type);
            break;
        case Float:
            if (composite->sign == Signed) {
                err = impl_float_type(unit, composite->scale, llvm_type);
            } else {
                ERROR("unsgined floating point not supported");
                err = new_backend_impl_error(
                    Implementation, composite->nodePtr,
                    "unsigned floating-point not supported");
            }
            break;
        default:
            PANIC("invalid primitive kind: %ld", composite->primitive);
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
    DEBUG("retrieving type implementation...");
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
        default:
            PANIC("invalid type kind: %ld", gemstone_type->kind);
            break;
    }

    return err;
}

BackendError impl_box_type(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                           BoxType* box, LLVMTypeRef* llvm_type) {
    DEBUG("implementing box type...");
    GHashTableIter iterator;
    g_hash_table_iter_init(&iterator, box->member);

    gpointer key = NULL;
    gpointer val = NULL;

    BackendError err;

    GArray* members = g_array_new(FALSE, FALSE, sizeof(LLVMTypeRef));

    DEBUG("implementing box members...");
    while (g_hash_table_iter_next(&iterator, &key, &val) != FALSE) {
        Type* member_type = ((BoxMember*)val)->type;

        DEBUG("implementing member: %s ", ((BoxMember*)val)->name);

        LLVMTypeRef llvm_type = NULL;
        err = get_type_impl(unit, scope, member_type, &llvm_type);

        if (err.kind != Success) {
            break;
        }

        g_array_append_val(members, llvm_type);
    }
    DEBUG("implemented %ld members", members->len);

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
    DEBUG("implementing reference type...");
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
    DEBUG("implementing type of kind: %ld as %s", gemstone_type->kind, alias);

    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope, gemstone_type, &llvm_type);

    if (err.kind == Success) {
        g_hash_table_insert(scope->types, (gpointer)alias, llvm_type);
    }

    return err;
}

BackendError impl_types(LLVMBackendCompileUnit* unit, LLVMGlobalScope* scope,
                        GHashTable* types) {
    DEBUG("implementing given types of %p", types);
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
    DEBUG("building primitive %ld default value", type);
    switch (type) {
        case Int:
            *llvm_value = LLVMConstIntOfString(llvm_type, "0", 10);
            break;
        case Float:
            *llvm_value = LLVMConstRealOfString(llvm_type, "0");
            break;
        default:
            ERROR("invalid primitive type: %ld", type);
            return new_backend_impl_error(Implementation, NULL,
                                          "unknown primitive type");
    }

    return SUCCESS;
}

BackendError get_composite_default_value(CompositeType* composite,
                                         LLVMTypeRef llvm_type,
                                         LLVMValueRef* llvm_value) {
    DEBUG("building composite default value");
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
            ERROR("invalid primitive type: %ld", composite->primitive);
            break;
    }

    return err;
}

BackendError get_reference_default_value(LLVMTypeRef llvm_type,
                                         LLVMValueRef* llvm_value) {
    DEBUG("building reference default value");
    *llvm_value = LLVMConstPointerNull(llvm_type);

    return SUCCESS;
}

BackendError get_box_default_value(LLVMBackendCompileUnit* unit,
                                   LLVMGlobalScope* scope, BoxType* type,
                                   LLVMTypeRef llvm_type,
                                   LLVMValueRef* llvm_value) {
    DEBUG("building box default value...");
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

    DEBUG("build %ld member default values", constants->len);

    *llvm_value = LLVMConstNamedStruct(
        llvm_type, (LLVMValueRef*)constants->data, constants->len);

    g_array_free(constants, FALSE);

    return err;
}

BackendError get_type_default_value(LLVMBackendCompileUnit* unit,
                                    LLVMGlobalScope* scope, Type* gemstone_type,
                                    LLVMValueRef* llvm_value) {
    BackendError err = new_backend_impl_error(
        Implementation, gemstone_type->nodePtr, "No default value for type");

    LLVMTypeRef llvm_type = NULL;
    err = get_type_impl(unit, scope, gemstone_type, &llvm_type);
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
                                        llvm_type, llvm_value);
            break;
        default:
            PANIC("invalid type kind: %ld", gemstone_type->kind);
            break;
    }

    return err;
}
