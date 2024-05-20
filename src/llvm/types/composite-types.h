
#ifndef LLVM_TYPES_COMPOSITE_TYPES_H_
#define LLVM_TYPES_COMPOSITE_TYPES_H_

#define BITS_PER_BYTE 8

enum Sign_t {
    Signed = 1,
    Unsigned = -1
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
    enum Sign_t sign;
    enum Scale_t scale;
    enum Primitive_t prim;
} Composite;

typedef struct CompositeType_t* CompositeRef;


#endif // LLVM_TYPES_COMPOSITE_TYPES_H_