
#ifndef LLVM_TYPE_H_
#define LLVM_TYPE_H_

#include <ast/ast.h>
#include <sys/log.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm/types/composite-types.h>
#include <llvm/types/scope.h>

#define INVALID_COMPOSITE NULL

LLVMTypeRef llvm_type_from_composite(LLVMContextRef context, const CompositeRef composite);

struct CompositeType_t ast_type_to_composite(const TypeScopeRef scope, AST_NODE_PTR type);

#endif // LLVM_TYPE_H_
