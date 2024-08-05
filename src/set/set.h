#ifndef _SET_H_
#define _SET_H_

#include <ast/ast.h>
#include <set/types.h>

#define SEMANTIC_OK    0
#define SEMANTIC_ERROR 1

// type of string literal
extern const Type StringLiteralType;

Module* create_set(AST_NODE_PTR rootNodePtr);

void delete_set(Module* module);

bool compareTypes(Type* leftType, Type* rightType);

#endif
