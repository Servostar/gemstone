//
// Created by servostar on 5/8/24.
//

#include <codegen/backend.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

static CodegenResult __new_result(enum CodegenErrorCode code, char* message) {
    const CodegenResult result = {
        .code = code,
        .message =  message
    };
    return result;
}

static struct Codegen_Lib_t {
    CodegenFunction codegen_function;
    InitCodegenBackend init_function;
    DeinitCodegenBackend deinit_function;
    unsigned char initialized;
} CodegenLib;

static CodegenResult __unimplemented() {
    return __new_result(Unimplemented, "Backend not set");
}

static CodegenResult __default_codegen_function(AST_NODE_PTR _ptr, void** _out) {
    return __unimplemented();
}

static CodegenResult __default_backend_init(void) {
    return __unimplemented();
}

static CodegenResult __default_backend_deinit(void) {
    return __unimplemented();
}

static void __deinit(void) {
    if (CodegenLib.initialized && CodegenLib.deinit_function) {
        CodegenLib.deinit_function();
    }
}

void CG_init(void) {
    CodegenLib.codegen_function = __default_codegen_function;
    CodegenLib.init_function = __default_backend_init;
    CodegenLib.deinit_function = __default_backend_deinit;

    atexit(__deinit);
}

void CG_set_codegen_backend(InitCodegenBackend init, DeinitCodegenBackend deinit, CodegenFunction codegen) {
    CodegenLib.codegen_function = codegen;
    CodegenLib.init_function = init;
    CodegenLib.deinit_function = deinit;
}

CodegenResult CG_codegen(const AST_NODE_PTR ast, void** output) {
    if (ast == NULL) {
        return __new_result(InvalidAST, "AST is NULL");
    }

    if (output == NULL) {
        return __new_result(NoOutputStorage, "Output pointer is NULL");
    }

    if (CodegenLib.codegen_function == NULL) {
        return __new_result(NoBackend, "No code generation backend");
    }

    if (!CodegenLib.initialized && CodegenLib.init_function) {
        const CodegenResult result = CodegenLib.init_function();

        if (result.code != Sucess) {
            return result;
        }

        CodegenLib.initialized = TRUE;
    }

    return CodegenLib.codegen_function(ast, output);
}