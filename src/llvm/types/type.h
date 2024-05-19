
#ifndef GEMSTONE_TYPE_H_
#define GEMSTONE_TYPE_H_

#include <ast/ast.h>
#include <llvm/types/composite.h>

enum GemstoneTypeKind_t {
    TypeComposite,
    TypeReference,
    TypeBox
};

struct GemstoneType_t;

typedef struct GemstoneRefType_t {
    struct GemstoneType_t* type;
} GemstoneRefType;

typedef struct GemstoneType_t {
    enum GemstoneTypeKind_t kind;
    union GemstoneTypeSpecs_t {
        Composite composite;
        GemstoneRefType reference;
    } specs;
} GemstoneType;

struct TypeScope_t;

typedef struct TypeScope_t {
    // TODO: array of child scopes
    // TODO: array of types
} TypeScope;

/**
 * @brief Convert a type declaration into a concrete type.
 * 
 * @param type A type declaration (either identifier or composite)
 * @return GemstoneType
 */
GemstoneType get_type_from_ast(const AST_NODE_PTR type);

#endif // GEMSTONE_TYPE_H_
