
#include <alloca.h>
#include <codegen/backend.h>
#include <llvm/backend.h>
#include <sys/log.h>
#include <set/types.h>

[[gnu::always_inline]]
inline Variable* create_variable_decl(const char* name, StorageQualifier qualifier, Type* type) {
    Variable* variable = alloca(sizeof(Variable));

    variable->name = name;
    variable->kind = VariableKindDeclaration;
    variable->nodePtr = NULL;
    variable->impl.declaration.nodePtr = NULL;
    variable->impl.declaration.qualifier = qualifier;
    variable->impl.declaration.type = type;

    return variable;
}

Module* create_module() {
    Module* module = malloc(sizeof(Module));

    module->boxes = g_hash_table_new(g_str_hash, g_str_equal);
    module->functions = g_hash_table_new(g_str_hash, g_str_equal);
    module->imports = g_array_new(FALSE, FALSE, sizeof(Type));
    module->variables = g_hash_table_new(g_str_hash, g_str_equal);
    module->types = g_hash_table_new(g_str_hash, g_str_equal);

    Type type_int;
    type_int.kind = TypeKindPrimitive;
    type_int.impl.primitive = Int;

    g_hash_table_insert(module->variables, "a", create_variable_decl("a", Global, &type_int));

    Type type_composite;
    type_composite.kind = TypeKindComposite;
    type_composite.impl.composite.primitive = Float;
    type_composite.impl.composite.scale = 2.0;
    type_composite.impl.composite.sign = Signed;

    g_hash_table_insert(module->variables, "b", create_variable_decl("b", Global, &type_composite));

    Type type_box;
    type_box.kind = TypeKindBox;
    type_box.impl.box->member = g_hash_table_new(g_str_hash, g_str_equal);

    BoxMember* member1 = alloca(sizeof(BoxMember));
    member1->box = NULL;
    member1->name = "foo";
    member1->type = alloca(sizeof(Type));
    *(member1->type) = type_int;

    g_hash_table_insert(type_box.impl.box->member, "foo", member1);

    Type type_half;
    type_half.kind = TypeKindComposite;
    type_half.impl.composite.primitive = Float;
    type_half.impl.composite.scale = 0.5;
    type_half.impl.composite.sign = Signed;

    BoxMember* member2 = alloca(sizeof(BoxMember));
    member2->box = NULL;
    member2->name = "bar";
    member2->type = alloca(sizeof(Type));
    *(member2->type) = type_half;

    g_hash_table_insert(type_box.impl.box->member, "bar", member2);

    g_hash_table_insert(module->variables, "c", create_variable_decl("c", Global, &type_box));

    Type type_reference;
    type_reference.kind = TypeKindReference;
    type_reference.impl.reference = &type_box;

    g_hash_table_insert(module->variables, "d", create_variable_decl("d", Global, &type_reference));

    return module;
}

int main(int argc, char* argv[]) {
    parse_options(argc, argv);
    log_init();

    // no need to clean up ;-)
    Module* module = create_module();

    llvm_backend_init();

    BackendError err;
    err = init_backend();
    if (err.kind != Success) {
        PANIC("%ld: at [%p] %s", err.kind, err.impl.ast_node, err.impl.message);
    }

    TargetConfig* config = default_target_config();

    err = generate_code(module, config);
    if (err.kind != Success) {
        PANIC("%ld: at [%p] %s", err.kind, err.impl.ast_node, err.impl.message);
    }

    delete_target_config(config);

    err = deinit_backend();
    if (err.kind != Success) {
        PANIC("%ld: at [%p] %s", err.kind, err.impl.ast_node, err.impl.message);
    }
}
