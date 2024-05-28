//
// Created by servostar on 5/28/24.
//

#include <llvm/expr.h>
#include <llvm/types.h>

BackendError impl_bitwise_operation(LLVMBackendCompileUnit *unit,
                                    LLVMLocalScope *scope,
                                    LLVMBuilderRef builder,
                                    Operation *operation,
                                    LLVMValueRef *llvm_result) {
    // TODO: resolve lhs and rhs or op
    LLVMValueRef rhs = NULL;
    LLVMValueRef lhs = NULL;
    LLVMValueRef op = NULL;

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

BackendError impl_logical_operation(LLVMBackendCompileUnit *unit,
                                    LLVMLocalScope *scope,
                                    LLVMBuilderRef builder,
                                    Operation *operation,
                                    LLVMValueRef *llvm_result) {
    // TODO: resolve lhs and rhs or op
    LLVMValueRef rhs = NULL;
    LLVMValueRef lhs = NULL;
    LLVMValueRef op = NULL;

    if (operation->kind == BitwiseNot) {
        // single operand
        op = convert_integral_to_boolean(builder, op);
    } else {
        // two operands
        lhs = convert_integral_to_boolean(builder, lhs);
        rhs = convert_integral_to_boolean(builder, rhs);
    }

    switch (operation->impl.bitwise) {
        case LogicalAnd:
            // TODO: convert to either 0 or 1
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

BackendError impl_operation(LLVMBackendCompileUnit *unit, LLVMLocalScope *scope,
                            LLVMBuilderRef builder, Operation *operation,
                            LLVMValueRef *llvm_result) {
    switch (operation->kind) {
        case Bitwise:
            impl_bitwise_operation(unit, scope, builder, operation,
                                   llvm_result);
            break;
        case Logical:
            impl_logical_operation(unit, scope, builder, operation,
                                   llvm_result);
            break;
    }
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
            err = get_type_value(unit, scope->func_scope->global_scope,
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