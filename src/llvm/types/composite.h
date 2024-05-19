
#ifndef LLVM_TYPE_H_
#define LLVM_TYPE_H_

#include <ast/ast.h>
#include <sys/log.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#define BITS_PER_BYTE 8

enum Sign_t {
    Signed,
    Unsigned
};

enum Scale_t {
    ATOM = 1,
    HALF = 2,
    SINGLE = 4,
    DOUBLE = 8,
    QUAD = 16,
    OCTO = 32
};

enum Primitive_t {
    Int,
    Float
};

typedef struct CompositeType_t {
    const char* name;
    enum Sign_t sign;
    enum Scale_t scale;
    enum Primitive_t prim;
} Composite;

typedef struct TypeScope_t {
    // vector of composite data types
    Composite* composites;
    size_t composite_len;
    size_t composite_cap;
} TypeScope;

typedef struct CompositeType_t* CompositeRef;

#define INVALID_COMPOSITE NULL

LLVMTypeRef llvm_type_from_composite(LLVMContextRef context, const CompositeRef composite);

struct CompositeType_t ast_type_to_composite(AST_NODE_PTR type);

#endif // LLVM_TYPE_H_
