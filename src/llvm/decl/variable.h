
#ifndef LLVM_DECL_VAR_H_
#define LLVM_DECL_VAR_H_

#include <codegen/backend.h>
#include <ast/ast.h>
#include <llvm-c/Types.h>
#include <llvm/types/structs.h>
#include <llvm/types/scope.h>
#include <llvm-c/Core.h>

GArray* declaration_from_ast(TypeScopeRef scope, const AST_NODE_PTR node);

BackendError llvm_create_declaration(LLVMModuleRef llvm_module, LLVMBuilderRef llvm_builder, GemstoneDeclRef gem_decl, LLVMValueRef* llvm_decl);

#endif // LLVM_DECL_VAR_H_
