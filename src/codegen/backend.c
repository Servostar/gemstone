
#include <codegen/backend.h>
#include <sys/log.h>

static struct CodegenBackend_t {
    codegen_init init_func;
    codegen_deinit deinit_func;
    codegen codegen_func;
    const char* name;
} CodegenBackend;

BackendError init_backend(void) {
    DEBUG("initializing backend: %s", CodegenBackend.name);

    if (CodegenBackend.init_func == NULL) {
        ERROR("backend: %s is not properly initialized", CodegenBackend.name);
        return NoBackend;
    }

    size_t code = CodegenBackend.init_func();

    if (code) {
        ERROR("failed to initialize backend: %s with code: %ld", CodegenBackend.name, code);
        return Other;
    }

    return Success;
}

BackendError deinit_backend(void) {
    DEBUG("undoing initializing of backend: %s", CodegenBackend.name);

    if (CodegenBackend.deinit_func == NULL) {
        ERROR("backend: %s is not properly initialized", CodegenBackend.name);
        return NoBackend;
    }

    size_t code = CodegenBackend.deinit_func();

    if (code) {
        ERROR("failed to undo initialization of backend: %s with code: %ld", CodegenBackend.name, code);
        return Other;
    }

    return Success;
}

BackendError set_backend(const codegen_init init_func, const codegen_deinit deinit_func, const codegen codegen_func, const char* name) {
    CodegenBackend.init_func = init_func;
    CodegenBackend.deinit_func = deinit_func;
    CodegenBackend.codegen_func = codegen_func;
    CodegenBackend.name = name;

    return Success;
}

BackendError generate_code(const AST_NODE_PTR root, void** output) {
    DEBUG("generating code with backend: %s", CodegenBackend.name);

    if (CodegenBackend.codegen_func == NULL) {
        ERROR("backend: %s is not properly initialized", CodegenBackend.name);
        return NoBackend;
    }

    size_t code = CodegenBackend.codegen_func(root, output);
    if (code) {
        ERROR("code generation of backend: %s failed with code: %ld", CodegenBackend.name, code);
        return Other;
    }

    return Success;
}
