//
// Created by servostar on 8/4/24.
//

#include <assert.h>
#include <set/types.h>
#include <sys/log.h>

Type* SET_function_get_return_type(Function* function) {
    assert(NULL != function);

    const Type* return_type = NULL;

    switch (function->kind) {
        case FunctionDeclarationKind:
            return_type = function->impl.declaration.return_value;
            break;
        case FunctionDefinitionKind:
            return_type = function->impl.definition.return_value;
            break;
        default:
            PANIC("invalid function kind: %d", function->kind);
    }

    if (NULL == return_type) {
        ERROR("Function return type is nullptr");
    }
}
