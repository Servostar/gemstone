//
// Created by servostar on 5/28/24.
//

#include <llvm/llvm-ir/expr.h>
#include <llvm/llvm-ir/types.h>
#include <sys/log.h>

BackendError impl_bitwise_operation([[maybe_unused]] LLVMBackendCompileUnit *unit,
                                    [[maybe_unused]] LLVMLocalScope *scope,
                                    LLVMBuilderRef builder,
                                    Operation *operation,
                                    LLVMValueRef *llvm_result) {
    // TODO: resolve lhs and rhs or op
    LLVMValueRef rhs = NULL;
    LLVMValueRef lhs = NULL;

    if (operation->impl.bitwise == BitwiseNot) {
        // single operand
    } else {
        // two operands
    }

    switch (operation->impl.bitwise) {
        case BitwiseAnd:
            *llvm_result = LLVMBuildAnd(builder, lhs, rhs, "bitwise and");
            break;
        case BitwiseOr:
            *llvm_result = LLVMBuildOr(builder, lhs, rhs, "bitwise or");
            break;
        case BitwiseXor:
            *llvm_result = LLVMBuildXor(builder, lhs, rhs, "bitwise xor");
            break;
        case BitwiseNot:
            *llvm_result = LLVMBuildNot(builder, rhs, "bitwise not");
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
static LLVMValueRef convert_integral_to_boolean(
        LLVMBuilderRef builder, LLVMValueRef integral) {
    // type of input
    LLVMTypeRef valueType = LLVMTypeOf(integral);
    // zero value of same type as integral
    LLVMValueRef zero = LLVMConstIntOfString(valueType, "0", 10);
    // returns 1 if integral is not zero and zero otherwise
    return LLVMBuildICmp(builder, LLVMIntNE, zero, integral, "to boolean");
}

BackendError impl_logical_operation([[maybe_unused]] LLVMBackendCompileUnit *unit,
                                    [[maybe_unused]] LLVMLocalScope *scope,
                                    LLVMBuilderRef builder,
                                    Operation *operation,
                                    LLVMValueRef *llvm_result) {
    // TODO: resolve lhs and rhs or op
    LLVMValueRef rhs = NULL;
    LLVMValueRef lhs = NULL;

    if (operation->impl.logical == LogicalNot) {
        // single operand
        rhs = convert_integral_to_boolean(builder, rhs);
    } else {
        // two operands
        lhs = convert_integral_to_boolean(builder, lhs);
        rhs = convert_integral_to_boolean(builder, rhs);
    }

    switch (operation->impl.logical) {
        case LogicalAnd:
            *llvm_result = LLVMBuildAnd(builder, lhs, rhs, "logical and");
            break;
        case LogicalOr:
            *llvm_result = LLVMBuildOr(builder, lhs, rhs, "logical or");
            break;
        case LogicalXor:
            *llvm_result = LLVMBuildXor(builder, lhs, rhs, "logical xor");
            break;
        case LogicalNot:
            *llvm_result = LLVMBuildNot(builder, rhs, "logical not");
            break;
    }

    return SUCCESS;
}

static LLVMBool is_floating_point(LLVMValueRef value) {
    LLVMTypeRef valueType = LLVMTypeOf(value);
    LLVMTypeKind typeKind = LLVMGetTypeKind(valueType);

    return typeKind == LLVMFloatTypeKind || typeKind == LLVMHalfTypeKind || typeKind == LLVMDoubleTypeKind ||
           typeKind == LLVMFP128TypeKind;
}

static LLVMBool is_integral(LLVMValueRef value) {
    LLVMTypeRef valueType = LLVMTypeOf(value);
    LLVMTypeKind typeKind = LLVMGetTypeKind(valueType);

    return typeKind == LLVMIntegerTypeKind;
}

BackendError impl_relational_operation([[maybe_unused]] LLVMBackendCompileUnit *unit,
                                       [[maybe_unused]] LLVMLocalScope *scope,
                                       LLVMBuilderRef builder,
                                       Operation *operation,
                                       LLVMValueRef *llvm_result) {
    // TODO: resolve lhs and rhs or op
    LLVMValueRef rhs = NULL;
    LLVMValueRef lhs = NULL;

    if ((is_integral(lhs) && is_integral(rhs)) == 1) {
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

        *llvm_result = LLVMBuildICmp(builder, operator, lhs, rhs, "integral comparison");
    } else if ((is_floating_point(lhs) && is_floating_point(rhs)) == 1) {
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

        *llvm_result = LLVMBuildFCmp(builder, operator, lhs, rhs, "floating point comparison");
    } else {
        PANIC("invalid type for relational operator");
    }

    return SUCCESS;
}

BackendError impl_arithmetic_operation([[maybe_unused]] LLVMBackendCompileUnit *unit,
                                       [[maybe_unused]] LLVMLocalScope *scope,
                                       LLVMBuilderRef builder,
                                       Operation *operation,
                                       LLVMValueRef *llvm_result) {
    // TODO: resolve lhs and rhs or op
    LLVMValueRef rhs = NULL;
    LLVMValueRef lhs = NULL;

    if ((is_integral(lhs) && is_integral(rhs)) == 1) {
        // integral type

        switch (operation->impl.arithmetic) {
            case Add:
                *llvm_result = LLVMBuildNSWAdd(builder, lhs, rhs, "signed integer addition");
                break;
            case Sub:
                *llvm_result = LLVMBuildNSWSub(builder, lhs, rhs, "signed integer subtraction");
                break;
            case Mul:
                *llvm_result = LLVMBuildNSWMul(builder, lhs, rhs, "signed integer multiply");
                break;
            case Div:
                *llvm_result = LLVMBuildSDiv(builder, lhs, rhs, "signed integer divide");
                break;
        }

    } else if ((is_floating_point(lhs) && is_floating_point(rhs)) == 1) {
        // integral type
        LLVMRealPredicate operator = 0;

        switch (operation->impl.arithmetic) {
            case Add:
                *llvm_result = LLVMBuildFAdd(builder, lhs, rhs, "floating point addition");
                break;
            case Sub:
                *llvm_result = LLVMBuildFSub(builder, lhs, rhs, "floating point subtraction");
                break;
            case Mul:
                *llvm_result = LLVMBuildFMul(builder, lhs, rhs, "floating point multiply");
                break;
            case Div:
                *llvm_result = LLVMBuildFDiv(builder, lhs, rhs, "floating point divide");
                break;
        }

        *llvm_result = LLVMBuildFCmp(builder, operator, lhs, rhs, "floating point comparison");
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
    // TODO: resolve sub expression
    LLVMValueRef operand = NULL;
    LLVMTypeRef target_type = NULL;
    BackendError err = get_type_impl(unit, scope->func_scope->global_scope,
                                     &transmute->targetType, &target_type);
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
    // TODO: resolve sub expression
    LLVMValueRef operand = NULL;
    LLVMTypeRef target_type = NULL;
    BackendError err = get_type_impl(unit, scope->func_scope->global_scope,
                                     &typecast->targetType, &target_type);
    // if target type is valid
    if (err.kind != Success) {
        return err;
    }

    LLVMBool dst_signed = is_type_signed(&typecast->targetType);
    // TODO: derive source type sign
    const LLVMOpcode opcode =
            LLVMGetCastOpcode(operand, 0, target_type, dst_signed);
    *llvm_result =
            LLVMBuildCast(builder, opcode, operand, target_type, "transmute");

    return err;
}

BackendError impl_expr(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                       LLVMBuilderRef builder, Expression *expr,
                       LLVMValueRef *llvm_result) {
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
    }

    return err;
}
