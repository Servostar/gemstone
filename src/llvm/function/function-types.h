
#ifndef LLVM_TYPES_FUNCTION_TYPES_H_
#define LLVM_TYPES_FUNCTION_TYPES_H_

#include <llvm-c/Types.h>
#include <llvm/types/structs.h>
#include <glib.h>

enum IO_Qualifier_t {
    Unspec,
    In,
    Out,
    InOut
};

typedef struct GemstoneParam_t {
    const char* name;
    enum IO_Qualifier_t qualifier;
    GemstoneTypeRef typename;
} GemstoneParam;

typedef struct GemstoneFun_t {
    const char* name;
    GArray* params;
    LLVMTypeRef llvm_signature;
    LLVMValueRef llvm_function;
} GemstoneFun;

typedef GemstoneFun* GemstoneFunRef;

#endif // LLVM_TYPES_FUNCTION_TYPES_H_
