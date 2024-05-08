//
// Created by servostar on 5/8/24.
//

#ifndef LIB_H
#define LIB_H

#include <ast/ast.h>

enum CodegenErrorCode {
    Sucess,
    Unimplemented,
    InvalidAST,
    NoBackend,
    NoOutputStorage,
    Error
};

typedef struct CodegenResult_t {
    const enum CodegenErrorCode code;
    const char* message;
} CodegenResult;

// parses a syntax tree into intermediate representation
// and creates backend specific data structure
// returns a success code
typedef CodegenResult (*CodegenFunction) (const AST_NODE_PTR, void**);

typedef CodegenResult (*InitCodegenBackend) (void);

typedef CodegenResult (*DeinitCodegenBackend) (void);

void CG_init(void);

// Should only be called from a backend implementation
void CG_set_codegen_backend(InitCodegenBackend init, DeinitCodegenBackend deinit, CodegenFunction codegen);

CodegenResult CG_codegen(AST_NODE_PTR, void**);

#endif //LIB_H
