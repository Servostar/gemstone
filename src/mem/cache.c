//
// Created by servostar on 6/5/24.
//

#include <assert.h>
#include <cfg/opt.h>
#include <glib.h>
#include <mem/cache.h>
#include <string.h>
#include <sys/log.h>

static GHashTable* namespaces = NULL;

typedef struct MemoryNamespaceStatistic_t {
    size_t bytes_allocated;
    size_t allocation_count;
    size_t reallocation_count;
    size_t manual_free_count;
    size_t faulty_reallocations;
    size_t faulty_allocations;
    size_t purged_free_count;
} MemoryNamespaceStatistic;

typedef enum MemoryBlockType_t {
    GenericBlock,
    GLIB_Array,
    GLIB_HashTable
} MemoryBlockType;

typedef struct MemoryBlock_t {
    void* block_ptr;
    MemoryBlockType kind;
} MemoryBlock;

typedef struct MemoryNamespace_t {
    MemoryNamespaceStatistic statistic;
    GArray* blocks;
} MemoryNamespace;

typedef MemoryNamespace* MemoryNamespaceRef;

static void
  namespace_statistics_print(MemoryNamespaceStatistic* memoryNamespaceStatistic,
                             char* name) {
    printf("Memory namespace statistics: `%s`\n", name);
    printf("------------------------------\n");
    printf("  allocated bytes:      %ld\n",
           memoryNamespaceStatistic->bytes_allocated);
    printf("  allocations:          %ld\n",
           memoryNamespaceStatistic->allocation_count);
    printf("  reallocations:        %ld\n",
           memoryNamespaceStatistic->reallocation_count);
    printf("  frees:                %ld\n",
           memoryNamespaceStatistic->manual_free_count);
    printf("  faulty allocations:   %ld\n",
           memoryNamespaceStatistic->faulty_allocations);
    printf("  faulty reallocations: %ld\n",
           memoryNamespaceStatistic->faulty_reallocations);
    printf("  purged allocations:   %ld\n",
           memoryNamespaceStatistic->purged_free_count);
    printf("\n");
}

static void* namespace_malloc(MemoryNamespaceRef memoryNamespace, size_t size) {
    assert(memoryNamespace != NULL);
    assert(size != 0);

    MemoryBlock block;
    block.block_ptr = malloc(size);
    block.kind      = GenericBlock;

    if (block.block_ptr == NULL) {
        memoryNamespace->statistic.faulty_allocations++;
    } else {
        g_array_append_val(memoryNamespace->blocks, block);

        memoryNamespace->statistic.allocation_count++;
        memoryNamespace->statistic.bytes_allocated += size;
    }

    return block.block_ptr;
}

static void namespace_free_block(MemoryBlock block) {
    switch (block.kind) {
        case GenericBlock:
            free(block.block_ptr);
            break;
        case GLIB_Array:
            g_array_free(block.block_ptr, TRUE);
            break;
        case GLIB_HashTable:
            g_hash_table_destroy(block.block_ptr);
            break;
    }
}

static gboolean namespace_free(MemoryNamespaceRef memoryNamespace,
                               void* block) {
    for (guint i = 0; i < memoryNamespace->blocks->len; i++) {
        MemoryBlock current_block =
          g_array_index(memoryNamespace->blocks, MemoryBlock, i);

        if (current_block.block_ptr == block) {
            assert(block != NULL);

            namespace_free_block(current_block);

            g_array_remove_index(memoryNamespace->blocks, i);

            memoryNamespace->statistic.manual_free_count++;

            return TRUE;
        }
    }
    return FALSE;
}

static void* namespace_realloc(MemoryNamespaceRef memoryNamespace, void* block,
                               size_t size) {
    void* reallocated_block = NULL;

    for (guint i = 0; i < memoryNamespace->blocks->len; i++) {
        MemoryBlock current_block =
          g_array_index(memoryNamespace->blocks, MemoryBlock, i);

        if (current_block.block_ptr == block) {
            reallocated_block = realloc(current_block.block_ptr, size);

            if (reallocated_block != NULL) {
                g_array_index(memoryNamespace->blocks, MemoryBlock, i)
                  .block_ptr = reallocated_block;
                memoryNamespace->statistic.bytes_allocated += size;
                memoryNamespace->statistic.reallocation_count++;
            } else {
                memoryNamespace->statistic.faulty_reallocations++;
            }

            break;
        }
    }

    return reallocated_block;
}

static void namespace_delete(MemoryNamespaceRef memoryNamespace) {
    g_array_free(memoryNamespace->blocks, TRUE);
    free(memoryNamespace);
}

static void namespace_purge(MemoryNamespaceRef memoryNamespace) {

    for (guint i = 0; i < memoryNamespace->blocks->len; i++) {
        MemoryBlock current_block =
          g_array_index(memoryNamespace->blocks, MemoryBlock, i);

        namespace_free_block(current_block);

        memoryNamespace->statistic.purged_free_count++;
    }

    g_array_remove_range(memoryNamespace->blocks, 0,
                         memoryNamespace->blocks->len);
}

static MemoryNamespaceRef namespace_new() {
    MemoryNamespaceRef memoryNamespace = malloc(sizeof(MemoryNamespace));

    memoryNamespace->blocks = g_array_new(FALSE, FALSE, sizeof(MemoryBlock));
    memoryNamespace->statistic.bytes_allocated      = 0;
    memoryNamespace->statistic.allocation_count     = 0;
    memoryNamespace->statistic.manual_free_count    = 0;
    memoryNamespace->statistic.faulty_reallocations = 0;
    memoryNamespace->statistic.faulty_allocations   = 0;
    memoryNamespace->statistic.purged_free_count    = 0;
    memoryNamespace->statistic.reallocation_count   = 0;

    return memoryNamespace;
}

GArray* namespace_new_g_array(MemoryNamespaceRef namespace, guint size) {
    MemoryBlock block;
    block.block_ptr = g_array_new(FALSE, FALSE, size);
    block.kind      = GLIB_Array;

    g_array_append_val(namespace->blocks, block);
    namespace->statistic.bytes_allocated += sizeof(GArray*);
    namespace->statistic.allocation_count++;

    return block.block_ptr;
}

GHashTable* namespace_new_g_hash_table(MemoryNamespaceRef namespace,
                                       GHashFunc hash_func,
                                       GEqualFunc key_equal_func) {
    MemoryBlock block;
    block.block_ptr = g_hash_table_new(hash_func, key_equal_func);
    block.kind      = GLIB_HashTable;

    g_array_append_val(namespace->blocks, block);
    namespace->statistic.bytes_allocated += sizeof(GHashTable*);
    namespace->statistic.allocation_count++;

    return block.block_ptr;
}

static void cleanup() {
    if (namespaces == NULL) {
        printf("==> Memory cache was unused <==\n");
        return;
    }

    GHashTableIter iter;
    char* name                         = NULL;
    MemoryNamespaceRef memoryNamespace = NULL;

    g_hash_table_iter_init(&iter, namespaces);

    while (g_hash_table_iter_next(&iter, (gpointer) &name,
                                  (gpointer) &memoryNamespace)) {
        assert(name != NULL);
        assert(memoryNamespace != NULL);

        namespace_purge(memoryNamespace);
        namespace_delete(memoryNamespace);
    }
}

void mem_init() {
    atexit(cleanup);
}

static MemoryNamespaceRef check_namespace(MemoryNamespaceName name) {
    if (namespaces == NULL) {
        namespaces = g_hash_table_new(g_str_hash, g_str_equal);
    }

    if (g_hash_table_contains(namespaces, name)) {

        return g_hash_table_lookup(namespaces, name);

    } else {
        MemoryNamespaceRef namespace = namespace_new();

        g_hash_table_insert(namespaces, name, namespace);

        return namespace;
    }
}

void* mem_alloc(MemoryNamespaceName name, size_t size) {
    MemoryNamespaceRef cache = check_namespace(name);

    if (cache == NULL) {
        PANIC("memory namespace not created");
    }

    return namespace_malloc(cache, size);
}

void* mem_realloc(MemoryNamespaceName name, void* ptr, size_t size) {
    MemoryNamespaceRef cache = check_namespace(name);

    if (cache == NULL) {
        PANIC("memory namespace not created");
    }

    return namespace_realloc(cache, ptr, size);
}

void mem_free_from(MemoryNamespaceName name, void* memory) {
    MemoryNamespaceRef cache = check_namespace(name);

    namespace_free(cache, memory);
}

void mem_free(void* memory) {
    GHashTableIter iter;
    char* name;
    MemoryNamespaceRef memoryNamespace;

    g_hash_table_iter_init(&iter, namespaces);
    while (g_hash_table_iter_next(&iter, (gpointer) &name,
                                  (gpointer) &memoryNamespace)) {

        if (namespace_free(memoryNamespace, memory)) {
            break;
        }
    }
}

void mem_purge_namespace(MemoryNamespaceName name) {
    if (g_hash_table_contains(namespaces, name)) {
        MemoryNamespaceRef cache = g_hash_table_lookup(namespaces, name);

        namespace_purge(cache);
    } else {
        WARN("purging invalid namespace: %s", name);
    }
}

char* mem_strdup(MemoryNamespaceName name, char* string) {
    return mem_clone(name, string, strlen(string) + 1);
}

void* mem_clone(MemoryNamespaceName name, void* data, size_t size) {
    void* clone = mem_alloc(name, size);

    memcpy(clone, data, size);

    return clone;
}

void print_memory_statistics() {
    GHashTableIter iter;
    char* name;
    MemoryNamespaceRef memoryNamespace;

    MemoryNamespaceStatistic total;
    total.bytes_allocated      = 0;
    total.faulty_reallocations = 0;
    total.faulty_allocations   = 0;
    total.manual_free_count    = 0;
    total.allocation_count     = 0;
    total.purged_free_count    = 0;
    total.reallocation_count   = 0;

    g_hash_table_iter_init(&iter, namespaces);
    while (g_hash_table_iter_next(&iter, (gpointer) &name,
                                  (gpointer) &memoryNamespace)) {

        namespace_statistics_print(&memoryNamespace->statistic, name);

        total.bytes_allocated += memoryNamespace->statistic.bytes_allocated;
        total.faulty_reallocations +=
          memoryNamespace->statistic.faulty_reallocations;
        total.faulty_allocations +=
          memoryNamespace->statistic.faulty_allocations;
        total.manual_free_count += memoryNamespace->statistic.manual_free_count;
        total.allocation_count += memoryNamespace->statistic.allocation_count;
        total.purged_free_count += memoryNamespace->statistic.purged_free_count;
        total.reallocation_count +=
          memoryNamespace->statistic.reallocation_count;
    }

    namespace_statistics_print(&total, "summary");

    printf("Note: untracked are memory allocations from external libraries and "
           "non-gc managed components.\n");
}

GArray* mem_new_g_array(MemoryNamespaceName name, guint element_size) {
    MemoryNamespaceRef cache = check_namespace(name);

    if (cache == NULL) {
        PANIC("memory namespace not created");
    }

    return namespace_new_g_array(cache, element_size);
}

GHashTable* mem_new_g_hash_table(MemoryNamespaceName name, GHashFunc hash_func,
                                 GEqualFunc key_equal_func) {
    MemoryNamespaceRef cache = check_namespace(name);

    if (cache == NULL) {
        PANIC("memory namespace not created");
    }

    return namespace_new_g_hash_table(cache, hash_func, key_equal_func);
}
