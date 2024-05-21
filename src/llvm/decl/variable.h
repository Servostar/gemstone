
#ifndef LLVM_DECL_VAR_H_
#define LLVM_DECL_VAR_H_

#include <ast/ast.h>
#include <llvm/types/structs.h>
#include <llvm/types/scope.h>

GArray* declaration_from_ast(TypeScopeRef scope, const AST_NODE_PTR node);

#endif // LLVM_DECL_VAR_H_
