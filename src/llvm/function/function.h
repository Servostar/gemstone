
#ifndef LLVM_FUNCTION_H_
#define LLVM_FUNCTION_H_

#include <ast/ast.h>
#include <llvm/types/type.h>

enum IO_Qualifier_t {
    Unspec,
    In,
    Out,
    InOut
};

typedef struct GemstoneParam_t {
    const char* name;
    enum IO_Qualifier_t qualifier;
    GemstoneType typename;
} GemstoneParam;

typedef struct GemstoneFun_t {
    const char* name;
    // TODO: add array of parameters
} GemstoneFun;

/**
 * @brief Convert an AST node into a function parameter struct
 * 
 * @param node the node starting a function parameter
 * @return GemstoneParam 
 */
GemstoneParam param_from_ast(const AST_NODE_PTR node);

/**
 * @brief Convert an AST node into a function
 * 
 * @param node the node starting a function
 * @return GemstoneFun 
 */
GemstoneFun fun_from_ast(const AST_NODE_PTR node);

#endif // LLVM_FUNCTION_H_
