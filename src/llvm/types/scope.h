
#ifndef LLVM_TYPE_SCOPE_H_
#define LLVM_TYPE_SCOPE_H_

#include <glib.h>
#include <llvm/types/structs.h>

typedef struct TypeScope_t TypeScope;

typedef TypeScope* TypeScopeRef;

/**
 * @brief Allocate a new type scope
 * 
 * @return TypeScopeRef 
 */
[[nodiscard("heap allocation")]]
TypeScopeRef type_scope_new();

/**
 * @brief Add a new type to this scope
 * 
 * @param scope 
 * @param type 
 */
[[gnu::nonnull(1)]]
void type_scope_append_type(TypeScopeRef scope, GemstoneTypedefRef type);

/**
 * @brief Add a new child scope to this scope
 * 
 * @param scope 
 * @param child_scope
 */
[[gnu::nonnull(1), gnu::nonnull(2)]]
void type_scope_append_scope(TypeScopeRef scope, TypeScopeRef child_scope);

/**
 * @brief Get the type at the specified index in this scope level
 * 
 * @param scope 
 * @param indx 
 */
[[gnu::nonnull(1)]]
GemstoneTypedefRef type_scope_get_type(TypeScopeRef scope, size_t indx);

/**
 * @brief Get the number of types in this scope level
 * 
 * @param scope 
 * @return size_t 
 */
[[gnu::nonnull(1)]]
size_t type_scope_types_len(TypeScopeRef scope);

/**
 * @brief Get the number of child scopes
 * 
 * @param scope 
 * @return size_t 
 */
[[gnu::nonnull(1)]]
size_t type_scope_scopes_len(TypeScopeRef scope);

/**
 * @brief Return a type inside this scope which matches the given name.
 * @attention Returns NULL if no type by this name is found. 
 *
 * @param name 
 * @return GemstoneTypedefRef 
 */
[[gnu::nonnull(1)]]
GemstoneTypedefRef type_scope_get_type_from_name(TypeScopeRef scope, const char* name);

/**
 * @brief Delete the scope. Deallocates all child scopes
 * 
 * @param scope 
 */
[[gnu::nonnull(1)]]
void type_scope_delete(TypeScopeRef scope);

#endif // LLVM_TYPE_SCOPE_H_
