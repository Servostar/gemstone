
#include <llvm/decl/variable.h>
#include <llvm/function/function-types.h>
#include <llvm/types/type.h>
#include <llvm/types/scope.h>
#include <string.h>

struct TypeScope_t {
    GArray* types;
    GArray* scopes;
    GArray* funcs;
    GHashTable* vars;
    TypeScopeRef parent;
};

TypeScopeRef type_scope_new() {
    TypeScopeRef scope = malloc(sizeof(TypeScope));

    // neither zero termination no initialisazion to zero needed
    scope->scopes = g_array_new(FALSE, FALSE, sizeof(TypeScopeRef));
    scope->types = g_array_new(FALSE, FALSE, sizeof(GemstoneTypedefRef));
    scope->funcs = g_array_new(FALSE, FALSE, sizeof(GemstoneFunRef));
    scope->vars = g_hash_table_new(g_str_hash, g_str_equal);
    scope->parent = NULL;

    return scope;
}

void type_scope_append_type(TypeScopeRef scope, GemstoneTypedefRef type) {
    g_array_append_val(scope->types, type);
}

void type_scope_append_scope(TypeScopeRef scope, TypeScopeRef child_scope) {
    g_array_append_val(scope->scopes, child_scope);
    child_scope->parent = scope;
}

GemstoneTypedefRef type_scope_get_type(TypeScopeRef scope, size_t index) {
    return ((GemstoneTypedefRef*) scope->types->data)[index];
}

size_t type_scope_types_len(TypeScopeRef scope) {
    return scope->types->len;
}

size_t type_scope_scopes_len(TypeScopeRef scope) {
    return scope->scopes->len;
}

GemstoneTypedefRef type_scope_get_type_from_name(TypeScopeRef scope, const char* name) {
    for (guint i = 0; i < scope->types->len; i++)  {
        GemstoneTypedefRef typeref = ((GemstoneTypedefRef*) scope->types->data)[i];

        if (strcmp(typeref->name, name) == 0) {
            return typeref;
        }
    }

    if (scope->parent == NULL) {
        return NULL;
    }

    return type_scope_get_type_from_name(scope->parent, name);
}

void type_scope_delete(TypeScopeRef scope) {

    for (guint i = 0; i < scope->scopes->len; i++) {
        TypeScopeRef scoperef = ((TypeScopeRef*) scope->scopes->data)[i];
        type_scope_delete(scoperef);
    }

    for (guint i = 0; i < scope->types->len; i++) {
        // TODO: free gemstone type
    }

    g_array_free(scope->scopes, TRUE);
    g_array_free(scope->types, TRUE);
    g_array_free(scope->funcs, TRUE);
    g_hash_table_destroy(scope->vars);

    free(scope);
}

void type_scope_add_variable(TypeScopeRef scope, GemstoneDeclRef decl) {
    g_hash_table_insert(scope->vars, (gpointer) decl->name, decl);
}

GemstoneDeclRef type_scope_get_variable(TypeScopeRef scope, const char* name) {
    if (g_hash_table_contains(scope->vars, name)) {
        return g_hash_table_lookup(scope->vars, name);
    }

    return NULL;
}

void type_scope_add_fun(TypeScopeRef scope, GemstoneFunRef function) {
    g_array_append_val(scope->funcs, function);
}

GemstoneFunRef type_scope_get_fun_from_name(TypeScopeRef scope, const char* name) {
    for (guint i = 0; i < scope->funcs->len; i++)  {
        GemstoneFunRef funref = ((GemstoneFunRef*) scope->funcs->data)[i];

        if (strcmp(funref->name, name) == 0) {
            return funref;
        }
    }

    if (scope->parent == NULL) {
        return NULL;
    }

    return type_scope_get_fun_from_name(scope->parent, name);
}
