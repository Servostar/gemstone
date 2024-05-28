//
// Created by servostar on 5/28/24.
//

#include <llvm/expr.h>
#include <llvm/types.h>

BackendError impl_transmute(LLVMBackendCompileUnit* unit, LLVMLocalScope* scope,
                            LLVMBuilderRef builder, Transmute* transmute,
                            LLVMValueRef* llvm_result) {
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

static LLVMBool is_type_signed(const Type* type) {
    switch (type->kind) {
        case TypeKindPrimitive:
            return 1;
        case TypeKindComposite:
            return type->impl.composite.sign == Signed;
        default:
            return 0;
    }
}

BackendError impl_typecast(LLVMBackendCompileUnit* unit, LLVMLocalScope* scope,
                           LLVMBuilderRef builder, TypeCast* typecast,
                           LLVMValueRef* llvm_result) {
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
BackendError impl_expr(LLVMBackendCompileUnit* unit, LLVMLocalScope* scope,
                       LLVMBuilderRef builder, Expression* expr,
                       LLVMValueRef* llvm_result) {
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

            break;
    }

    return err;
}