
#ifndef GEMSTONE_TYPE_H_
#define GEMSTONE_TYPE_H_

#include <ast/ast.h>
#include <llvm-c/Types.h>
#include <llvm/types/composite.h>
#include <llvm/types/structs.h>
#include <llvm/types/scope.h>

/**
 * @brief Convert a type declaration into a concrete type.
 * 
 * @param type A type declaration (either identifier or composite)
 * @return GemstoneType
 */
GemstoneTypeRef get_type_from_ast(const TypeScopeRef scope, const AST_NODE_PTR type);

/**
 * @brief Convert the type definition AST into a typedef reference
 * 
 * @param typdef 
 * @return GemstoneTypedefRef 
 */
GemstoneTypedefRef get_type_def_from_ast(const TypeScopeRef scope, const AST_NODE_PTR typdef);

/**
 * @brief Create an new typedefine reference
 * 
 * @param type 
 * @param name 
 * @return GemstoneTypedefRef 
 */
GemstoneTypedefRef new_typedefref(GemstoneTypeRef type, const char* name);

/**
 * @brief Create the LLVM function signature
 * 
 * @param context 
 * @param type 
 * @return LLVMTypeRef 
 */
LLVMTypeRef llvm_type_from_gemstone_type(LLVMContextRef context, GemstoneTypeRef type);

/**
 * @brief Free the type definition reference and its underlying type
 * 
 * @param ref 
 */
void delete_typedefref(GemstoneTypedefRef ref);

#endif // GEMSTONE_TYPE_H_
