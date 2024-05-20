
#ifndef LLVM_TYPE_STRUCTS_H_
#define LLVM_TYPE_STRUCTS_H_

#include <llvm/types/composite-types.h>

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

typedef GemstoneType* GemstoneTypeRef;

typedef struct GemstoneTypedef_t {
    const char* name;
    GemstoneTypeRef type;
} GemstoneTypedef;

typedef GemstoneTypedef* GemstoneTypedefRef;

#endif // LLVM_TYPE_STRUCTS_H_
