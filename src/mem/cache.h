//
// Created by servostar on 6/5/24.
//

#ifndef GEMSTONE_CACHE_H
#define GEMSTONE_CACHE_H

#include <mem/cache.h>
#include <stddef.h>
#include <glib.h>

typedef char* MemoryNamespaceName;

#define MemoryNamespaceAst "AST"
#define MemoryNamespaceLex "Lexer"
#define MemoryNamespaceLog "Logging"
#define MemoryNamespaceOpt "Options"
#define MemoryNamespaceTOML "TOML"
#define MemoryNamespaceSet "SET"
#define MemoryNamespaceLlvm "LLVM"
#define MemoryNamespaceLld "LLD"
#define MemoryNamespaceIo "I/O"
#define MemoryNamespaceStatic "Static"

/**
 * @brief Initialize the garbage collector.
 *        Must be done to ensure cleanup of memory.
 */
void mem_init();

/**
 * @brief Allocate a block of memory in the specified namespace.
 * @attention Must only be freed with mem_free() or mem_free_from()
 * @param name
 * @param size
 * @return pointer to the block
 */
void* mem_alloc(MemoryNamespaceName name, size_t size);

/**
 * @brief Reallocate a block of memory in the specified namespace.
 * @attention Must only be freed with mem_free() or mem_free_from()
 * @param name
 * @param size
 * @return pointer to the block
 */
void* mem_realloc(MemoryNamespaceName name, void *ptr, size_t size);

/**
 * @brief Free a block of memory from a specified namespace.
 *        Invoking multiple times on the same pointer will do nothing.
 * @param name
 * @param memory
 */
void mem_free_from(MemoryNamespaceName name, void* memory);

/**
 * @brief Free a block of memory.
 *        Invoking multiple times on the same pointer will do nothing.
 * @attention In case the namespace of the block is known, consider using mem_free_from()
 *            to avoid unnecessary overhead.
 * @param name
 * @param memory
 */
void mem_free(void* memory);

/**
 * @brief Delete all memory from the given namespace.
 * @param name
 */
void mem_purge_namespace(MemoryNamespaceName name);

/**
 * @brief Duplicate the given string with memory in the given namespace.
 * @param name
 * @param string
 * @return
 */
char* mem_strdup(MemoryNamespaceName name, char* string);

/**
 * @brief Duplicate the given block of data with memory in the given namespace.
 * @param name
 * @param data
 * @param size
 * @return
 */
void* mem_clone(MemoryNamespaceName name, void* data, size_t size);

void print_memory_statistics();

GArray* mem_new_g_array(MemoryNamespaceName name, guint element_size);

GHashTable* mem_new_g_hash_table(MemoryNamespaceName name, GHashFunc hash_func, GEqualFunc key_equal_func);

#endif //GEMSTONE_CACHE_H
