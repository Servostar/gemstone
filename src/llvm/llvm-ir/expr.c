//
// Created by servostar on 5/28/24.
//

#include <llvm/llvm-ir/expr.h>
#include <llvm/llvm-ir/types.h>
#include <sys/log.h>
#include <mem/cache.h>

BackendError impl_bitwise_operation(LLVMBackendCompileUnit *unit,
                                    LLVMLocalScope *scope,
                                    LLVMBuilderRef builder,
                                    Operation *operation,
                                    LLVMValueRef *llvm_result) {
    Expression *rhs = NULL;
    Expression *lhs = NULL;
    LLVMValueRef llvm_rhs = NULL;
    LLVMValueRef llvm_lhs = NULL;

    if (operation->impl.bitwise == BitwiseNot) {
        // single operand
        rhs = g_array_index(operation->operands, Expression*, 0);
        impl_expr(unit, scope, builder, rhs, FALSE, &llvm_rhs);
    } else {
        // two operands
        lhs = g_array_index(operation->operands, Expression*, 0);
        impl_expr(unit, scope, builder, lhs, FALSE, &llvm_lhs);

        rhs = g_array_index(operation->operands, Expression*, 1);
        impl_expr(unit, scope, builder, rhs, FALSE, &llvm_rhs);
    }

    switch (operation->impl.bitwise) {
        case BitwiseAnd:
            *llvm_result = LLVMBuildAnd(builder, llvm_lhs, llvm_rhs, "bitwise and");
            break;
        case BitwiseOr:
            *llvm_result = LLVMBuildOr(builder, llvm_lhs, llvm_rhs, "bitwise or");
            break;
        case BitwiseXor:
            *llvm_result = LLVMBuildXor(builder, llvm_lhs, llvm_rhs, "bitwise xor");
            break;
        case BitwiseNot:
            *llvm_result = LLVMBuildNot(builder, llvm_rhs, "bitwise not");
            break;
    }

    return SUCCESS;
}

/**
 * @brief Convert any integral type (integer) to a boolean value.
 *        A boolean value hereby meaning an integer of the same type as the input
 *        value but with the value of either 0 or one.
 * @param builder
 * @param integral
 * @return
 */
 [[maybe_unused]]
static LLVMValueRef convert_integral_to_boolean(
        LLVMBuilderRef builder, LLVMValueRef integral) {
    // type of input
    LLVMTypeRef valueType = LLVMTypeOf(integral);
    // zero value of same type as integral
    LLVMValueRef zero = LLVMConstIntOfString(valueType, "0", 10);
    // returns 1 if integral is not zero and zero otherwise
    return LLVMBuildICmp(builder, LLVMIntNE, zero, integral, "to boolean");
}

BackendError impl_logical_operation(LLVMBackendCompileUnit *unit,
                                    LLVMLocalScope *scope,
                                    LLVMBuilderRef builder,
                                    Operation *operation,
                                    LLVMValueRef *llvm_result) {
    Expression *rhs = NULL;
    Expression *lhs = NULL;
    LLVMValueRef llvm_rhs = NULL;
    LLVMValueRef llvm_lhs = NULL;

    if (operation->impl.logical == LogicalNot) {
        // single operand
        rhs = g_array_index(operation->operands, Expression*, 0);
        impl_expr(unit, scope, builder, rhs, FALSE, &llvm_rhs);
    } else {
        // two operands
        lhs = g_array_index(operation->operands, Expression*, 0);
        impl_expr(unit, scope, builder, lhs, FALSE, &llvm_lhs);

        rhs = g_array_index(operation->operands, Expression*, 1);
        impl_expr(unit, scope, builder, rhs, FALSE, &llvm_rhs);
    }

    switch (operation->impl.logical) {
        case LogicalAnd:
            *llvm_result = LLVMBuildAnd(builder, llvm_lhs, llvm_rhs, "logical and");
            break;
        case LogicalOr:
            *llvm_result = LLVMBuildOr(builder, llvm_lhs, llvm_rhs, "logical or");
            break;
        case LogicalXor:
            *llvm_result = LLVMBuildXor(builder, llvm_lhs, llvm_rhs, "logical xor");
            break;
        case LogicalNot:
            *llvm_result = LLVMBuildNot(builder, llvm_rhs, "logical not");
            break;
    }

    return SUCCESS;
}

static LLVMBool is_floating_point(Type *value) {
    if (value->kind == TypeKindPrimitive) {
        return value->impl.primitive == Float;
    }

    if (value->kind == TypeKindComposite) {
        return value->impl.composite.primitive == Float;
    }

    return FALSE;
}

static LLVMBool is_integral(Type *value) {
    if (value->kind == TypeKindPrimitive) {
        return value->impl.primitive == Int;
    }

    if (value->kind == TypeKindComposite) {
        return value->impl.composite.primitive == Int;
    }

    return FALSE;
}

BackendError impl_relational_operation(LLVMBackendCompileUnit *unit,
                                       LLVMLocalScope *scope,
                                       LLVMBuilderRef builder,
                                       Operation *operation,
                                       LLVMValueRef *llvm_result) {
    Expression *rhs = NULL;
    Expression *lhs = NULL;
    LLVMValueRef llvm_rhs = NULL;
    LLVMValueRef llvm_lhs = NULL;

    // two operands
    lhs = g_array_index(operation->operands, Expression*, 0);
    impl_expr(unit, scope, builder, lhs, FALSE, &llvm_lhs);

    rhs = g_array_index(operation->operands, Expression*, 1);
    impl_expr(unit, scope, builder, rhs, FALSE, &llvm_rhs);

    if (is_integral(rhs->result)) {
        // integral type
        LLVMIntPredicate operator = 0;

        switch (operation->impl.relational) {
            case Equal:
                operator = LLVMIntEQ;
                break;
            case Greater:
                operator = LLVMIntSGT;
                break;
            case Less:
                operator = LLVMIntSLT;
                break;
        }

        *llvm_result = LLVMBuildICmp(builder, operator, llvm_lhs, llvm_rhs, "integral comparison");

    } else if (is_floating_point(rhs->result)) {
        // integral type
        LLVMRealPredicate operator = 0;

        switch (operation->impl.relational) {
            case Equal:
                operator = LLVMRealOEQ;
                break;
            case Greater:
                operator = LLVMRealOGT;
                break;
            case Less:
                operator = LLVMRealOLT;
                break;
        }

        *llvm_result = LLVMBuildFCmp(builder, operator, llvm_lhs, llvm_rhs, "floating point comparison");
    } else {
        PANIC("invalid type for relational operator");
    }

    // *llvm_result = convert_integral_to_boolean(builder, *llvm_result);

    return SUCCESS;
}

BackendError impl_arithmetic_operation(LLVMBackendCompileUnit *unit,
                                       LLVMLocalScope *scope,
                                       LLVMBuilderRef builder,
                                       Operation *operation,
                                       LLVMValueRef *llvm_result) {
    Expression *rhs = NULL;
    Expression *lhs = NULL;
    LLVMValueRef llvm_rhs = NULL;
    LLVMValueRef llvm_lhs = NULL;

    if (operation->impl.arithmetic == Negate) {
        // single operand
        rhs = g_array_index(operation->operands, Expression*, 0);
        impl_expr(unit, scope, builder, rhs, FALSE, &llvm_rhs);
    } else {
        // two operands
        lhs = g_array_index(operation->operands, Expression*, 0);
        impl_expr(unit, scope, builder, lhs, FALSE, &llvm_lhs);

        rhs = g_array_index(operation->operands, Expression*, 1);
        impl_expr(unit, scope, builder, rhs, FALSE, &llvm_rhs);
    }

    if (is_integral(rhs->result)) {

        switch (operation->impl.arithmetic) {
            case Add:
                *llvm_result = LLVMBuildNSWAdd(builder, llvm_lhs, llvm_rhs, "signed integer addition");
                break;
            case Sub:
                *llvm_result = LLVMBuildNSWSub(builder, llvm_lhs, llvm_rhs, "signed integer subtraction");
                break;
            case Mul:
                *llvm_result = LLVMBuildNSWMul(builder, llvm_lhs, llvm_rhs, "signed integer multiply");
                break;
            case Div:
                *llvm_result = LLVMBuildSDiv(builder, llvm_lhs, llvm_rhs, "signed integer divide");
                break;
            case Negate:
                *llvm_result = LLVMBuildNeg(builder, llvm_rhs, "signed integer negate");
                break;
        }

    } else if (is_floating_point(rhs->result)) {

        switch (operation->impl.arithmetic) {
            case Add:
                *llvm_result = LLVMBuildFAdd(builder, llvm_lhs, llvm_rhs, "floating point addition");
                break;
            case Sub:
                *llvm_result = LLVMBuildFSub(builder, llvm_lhs, llvm_rhs, "floating point subtraction");
                break;
            case Mul:
                *llvm_result = LLVMBuildFMul(builder, llvm_lhs, llvm_rhs, "floating point multiply");
                break;
            case Div:
                *llvm_result = LLVMBuildFDiv(builder, llvm_lhs, llvm_rhs, "floating point divide");
                break;
            case Negate:
                *llvm_result = LLVMBuildFNeg(builder, llvm_rhs, "floating point negate");
                break;
        }

    } else {
        PANIC("invalid type for arithmetic operator");
    }

    return SUCCESS;
}

BackendError impl_operation(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                            LLVMBuilderRef builder, Operation *operation,
                            LLVMValueRef *llvm_result) {
    BackendError err;

    switch (operation->kind) {
        case Bitwise:
            err = impl_bitwise_operation(unit, scope, builder, operation,
                                         llvm_result);
            break;
        case Boolean:
            err = impl_logical_operation(unit, scope, builder, operation,
                                         llvm_result);
            break;
        case Relational:
            err = impl_relational_operation(unit, scope, builder, operation,
                                            llvm_result);
            break;
        case Arithmetic:
            err = impl_arithmetic_operation(unit, scope, builder, operation,
                                            llvm_result);
            break;
        default:
            PANIC("Invalid operator");
    }

    return err;
}

BackendError impl_transmute(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                            LLVMBuilderRef builder, Transmute *transmute,
                            LLVMValueRef *llvm_result) {

    LLVMValueRef operand = NULL;
    impl_expr(unit, scope, builder, transmute->operand, FALSE, &operand);

    LLVMTypeRef target_type = NULL;
    BackendError err = get_type_impl(unit, scope->func_scope->global_scope,
                                     transmute->targetType, &target_type);
    // if target type is valid
    if (err.kind == Success) {
        *llvm_result =
                LLVMBuildBitCast(builder, operand, target_type, "transmute");
    }

    return err;
}

static LLVMBool is_type_signed(const Type *type) {
    switch (type->kind) {
        case TypeKindPrimitive:
            return 1;
        case TypeKindComposite:
            return type->impl.composite.sign == Signed;
        default:
            return 0;
    }
}

BackendError impl_typecast(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                           LLVMBuilderRef builder, TypeCast *typecast,
                           LLVMValueRef *llvm_result) {
    LLVMValueRef operand = NULL;
    impl_expr(unit, scope, builder, typecast->operand, FALSE, &operand);

    LLVMTypeRef target_type = NULL;
    BackendError err = get_type_impl(unit, scope->func_scope->global_scope,
                                     typecast->targetType, &target_type);
    // if target type is valid
    if (err.kind != Success) {
        return err;
    }

    LLVMBool dst_signed = is_type_signed(typecast->targetType);
    LLVMBool src_signed = is_type_signed(typecast->operand->result);
    const LLVMOpcode opcode =
            LLVMGetCastOpcode(operand, src_signed, target_type, dst_signed);
    *llvm_result =
            LLVMBuildCast(builder, opcode, operand, target_type, "expr.typecast");

    return err;
}

BackendError impl_variable_load(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                                LLVMBuilderRef builder, Variable *variable,
                                LLVMBool reference,
                                LLVMValueRef *llvm_result) {

    LLVMValueRef llvm_variable = get_variable(scope, variable->name);

    Type* type;

    if (variable->kind == VariableKindDefinition) {
        type = variable->impl.definiton.declaration.type;
    } else {
        type = variable->impl.declaration.type;
    }

    if (llvm_variable == NULL) {
        return new_backend_impl_error(Implementation, NULL, "Variable not found");
    }

    if (reference) {
        // don't load in case variable is parameter?
        // only reference wanted

        *llvm_result = llvm_variable;

    } else {
        // no referencing, load value
        LLVMTypeRef llvm_type;

        get_type_impl(unit, scope->func_scope->global_scope, type, &llvm_type);

        if (LLVMGetTypeKind(LLVMTypeOf(llvm_variable)) == LLVMPointerTypeKind) {
            *llvm_result = LLVMBuildLoad2(builder, llvm_type, llvm_variable, "");
        } else {
            *llvm_result = llvm_variable;
        }
    }

    return SUCCESS;
}

BackendError impl_address_of(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                                LLVMBuilderRef builder, AddressOf* addressOf,
                                LLVMValueRef *llvm_result) {

    BackendError err = impl_expr(unit, scope, builder, addressOf->variable, FALSE, llvm_result);

    if (err.kind != Success) {
        return err;
    }

    return SUCCESS;
}

BackendError impl_deref(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                             LLVMBuilderRef builder, Dereference* dereference,
                             LLVMValueRef *llvm_result) {
    BackendError err;

    LLVMValueRef llvm_pointer = get_variable(scope, dereference->variable->impl.variable->name);
    LLVMTypeRef llvm_deref_type = NULL;
    err = get_type_impl(unit, scope->func_scope->global_scope, dereference->variable->result->impl.reference, &llvm_deref_type);
    if (err.kind != Success) {
        return err;
    }

    LLVMValueRef* index = mem_alloc(MemoryNamespaceLlvm, sizeof(LLVMValueRef));
    err = impl_expr(unit, scope, builder, dereference->index, FALSE, index);
    if (err.kind != Success) {
        return err;
    }

    *llvm_result = LLVMBuildGEP2(builder, llvm_deref_type, llvm_pointer, index, 1, "expr.deref.gep2");

    *llvm_result = LLVMBuildLoad2(builder, llvm_deref_type, *llvm_result, "expr.deref.load");

    return err;
}

BackendError impl_expr(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                       LLVMBuilderRef builder, Expression *expr,
                       LLVMBool reference,
                       LLVMValueRef *llvm_result) {
    DEBUG("implementing expression: %ld", expr->kind);
    BackendError err = SUCCESS;

    switch (expr->kind) {
        case ExpressionKindConstant:
            err = get_const_type_value(unit, scope->func_scope->global_scope,
                                       &expr->impl.constant, llvm_result);
            break;
        case ExpressionKindTransmute:
            err = impl_transmute(unit, scope, builder, &expr->impl.transmute,
                                 llvm_result);
            break;
        case ExpressionKindTypeCast:
            err = impl_typecast(unit, scope, builder, &expr->impl.typecast,
                                llvm_result);
            break;
        case ExpressionKindOperation:
            err = impl_operation(unit, scope, builder, &expr->impl.operation,
                                 llvm_result);
            break;
        case ExpressionKindVariable:
            err = impl_variable_load(unit, scope, builder, expr->impl.variable,
                                     reference,
                                     llvm_result);
            break;
        case ExpressionKindAddressOf:
            err = impl_address_of(unit, scope, builder, &expr->impl.addressOf,
                                  llvm_result);
            break;
        case ExpressionKindDereference:
            err = impl_deref(unit, scope, builder, &expr->impl.dereference,
                             llvm_result);
            break;
        default:
            err = new_backend_impl_error(Implementation, NULL, "unknown expression");
            break;
    }

    return err;
}
