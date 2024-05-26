
#include "set/types.h"
#include <codegen/backend.h>
#include <sys/log.h>

static struct CodegenBackend_t {
    codegen_init init_func;
    codegen_deinit deinit_func;
    codegen codegen_func;
    const char* name;
} CodegenBackend;

BackendError new_backend_error(BackendErrorKind kind) {
    return new_backend_impl_error(kind, NULL, NULL);
}

BackendError new_backend_impl_error(BackendErrorKind kind, AST_NODE_PTR node, const char* message) {
    BackendError error;
    error.kind = kind;
    error.impl.ast_node = node;
    error.impl.message = message;

    return error;
}

BackendError init_backend(void) {
    DEBUG("initializing backend: %s", CodegenBackend.name);

    if (CodegenBackend.init_func == NULL) {
        ERROR("backend: %s is not properly initialized", CodegenBackend.name);
        return new_backend_error(NoBackend);
    }

    BackendError code = CodegenBackend.init_func();

    if (code.kind != Success) {
        ERROR("failed to initialize backend: %s with code: %ld", CodegenBackend.name, code);
        return code;
    }

    return new_backend_error(Success);
}

BackendError deinit_backend(void) {
    DEBUG("undoing initializing of backend: %s", CodegenBackend.name);

    if (CodegenBackend.deinit_func == NULL) {
        ERROR("backend: %s is not properly initialized", CodegenBackend.name);
        return new_backend_error(NoBackend);
    }

    BackendError code = CodegenBackend.deinit_func();

    if (code.kind != Success) {
        ERROR("failed to undo initialization of backend: %s with code: %ld", CodegenBackend.name, code);
        return code;
    }

    return new_backend_error(Success);
}

BackendError set_backend(const codegen_init init_func, const codegen_deinit deinit_func, const codegen codegen_func, const char* name) {
    CodegenBackend.init_func = init_func;
    CodegenBackend.deinit_func = deinit_func;
    CodegenBackend.codegen_func = codegen_func;
    CodegenBackend.name = name;

    return new_backend_error(Success);
}

BackendError generate_code(const Module* root, void** output) {
    DEBUG("generating code with backend: %s", CodegenBackend.name);

    if (CodegenBackend.codegen_func == NULL) {
        ERROR("backend: %s is not properly initialized", CodegenBackend.name);
        return new_backend_error(NoBackend);
    }

    BackendError code = CodegenBackend.codegen_func(root, output);
    if (code.kind) {
        ERROR("code generation of backend: %s failed with code: %ld", CodegenBackend.name, code);
        return code;
    }

    return new_backend_error(Success);
}
