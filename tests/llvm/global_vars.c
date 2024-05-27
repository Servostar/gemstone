
#include <codegen/backend.h>
#include <llvm/backend.h>
#include <sys/log.h>
#include <set/types.h>

// NOTE: unused
AST_NODE_PTR root;

Module* create_module() {
    Module* module = malloc(sizeof(Module));

    module->boxes = g_hash_table_new(g_str_hash, g_str_equal);
    module->functions = g_hash_table_new(g_str_hash, g_str_equal);
    module->imports = g_array_new(FALSE, FALSE, sizeof(Type));
    module->variables = g_hash_table_new(g_str_hash, g_str_equal);
    module->types = g_hash_table_new(g_str_hash, g_str_equal);

    return module;
}

int main() {
    log_init();

    Module* module = create_module();

    llvm_backend_init();

    BackendError err;
    err = init_backend();
    if (err.kind != Success) {
        PANIC("%ld: at [%p] %s", err.kind, err.impl.ast_node, err.impl.message);
    }

    err = generate_code(module, NULL);
    if (err.kind != Success) {
        PANIC("%ld: at [%p] %s", err.kind, err.impl.ast_node, err.impl.message);
    }

    err = deinit_backend();
    if (err.kind != Success) {
        PANIC("%ld: at [%p] %s", err.kind, err.impl.ast_node, err.impl.message);
    }
}
