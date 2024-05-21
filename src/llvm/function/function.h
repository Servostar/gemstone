
#ifndef LLVM_FUNCTION_H_
#define LLVM_FUNCTION_H_

#include <ast/ast.h>
#include <llvm/function/function-types.h>
#include <llvm/types/scope.h>

/**
 * @brief Convert an AST node into a function parameter struct
 * 
 * @param node the node starting a function parameter
 * @return GemstoneParam 
 */
GemstoneParam param_from_ast(const TypeScopeRef scope, const AST_NODE_PTR node);

/**
 * @brief Convert an AST node into a function
 * 
 * @param node the node starting a function
 * @return GemstoneFunRef
 */
GemstoneFunRef fun_from_ast(const TypeScopeRef scope, const AST_NODE_PTR node);

/**
 * @brief Delete the given function
 * 
 * @param fun 
 */
void fun_delete(const GemstoneFunRef fun);

#endif // LLVM_FUNCTION_H_
