//
// Created by servostar on 5/7/24.
//

#include <gc/gc.h>
#include <sys/log.h>
#include <string.h>

#undef malloc
#undef free
#undef realloc

#define GC_HEAP_PREALLOC_CNT 10

typedef struct GC_Heap_Statistics_t {
    size_t bytes;
} GC_Heap_Statistics;

static struct GC_Heap_t {
    RAW_PTR* blocks;
    size_t cap;
    size_t len;
    GC_Heap_Statistics statistics;
} GC_GLOBAL_HEAP;

static void __GC_teardown(void) {
    INFO("Used %ld bytes in total", GC_GLOBAL_HEAP.statistics.bytes);

    if (GC_GLOBAL_HEAP.blocks == NULL) {
        return;
    }

    for (size_t i = 0; i < GC_GLOBAL_HEAP.len; i++) {
        free(GC_GLOBAL_HEAP.blocks[i]);
    }

    free(GC_GLOBAL_HEAP.blocks);

    GC_GLOBAL_HEAP.blocks = NULL;
}

static void __GC_check_init(void) {
    if (GC_GLOBAL_HEAP.blocks == NULL) {
        GC_GLOBAL_HEAP.cap = GC_HEAP_PREALLOC_CNT;
        const size_t bytes = sizeof(RAW_PTR) * GC_GLOBAL_HEAP.cap;
        GC_GLOBAL_HEAP.blocks = malloc(bytes);
        GC_GLOBAL_HEAP.len = 0;
    }
}

void GC_init(void) {
    atexit(__GC_teardown);
}

static void __GC_check_overflow(void) {
    if (GC_GLOBAL_HEAP.len >= GC_GLOBAL_HEAP.cap) {
        GC_GLOBAL_HEAP.cap += GC_HEAP_PREALLOC_CNT;
        const size_t bytes = sizeof(RAW_PTR) * GC_GLOBAL_HEAP.cap;
        GC_GLOBAL_HEAP.blocks = realloc(GC_GLOBAL_HEAP.blocks, bytes);
    }
}

RAW_PTR GC_malloc(const size_t bytes) {
    const RAW_PTR ptr = malloc(bytes);

    if (ptr == NULL) {
        FATAL("unable to allocate memory");
    }

    __GC_check_init();
    __GC_check_overflow();

    GC_GLOBAL_HEAP.blocks[GC_GLOBAL_HEAP.len++] = ptr;

    GC_GLOBAL_HEAP.statistics.bytes += bytes;

    return ptr;
}

static void __GC_swap_ptr(RAW_PTR old, RAW_PTR new) {
    for (size_t i = 0; i < GC_GLOBAL_HEAP.len; i++) {
        if (GC_GLOBAL_HEAP.blocks[i] == old) {
            GC_GLOBAL_HEAP.blocks[i] = new;
        }
    }
}

RAW_PTR GC_realloc(RAW_PTR ptr, size_t bytes) {
    const RAW_PTR new_ptr = (RAW_PTR) realloc(ptr, bytes);

    if (new_ptr == NULL) {
        FATAL("unable to reallocate memory");
    }

    __GC_swap_ptr(ptr, new_ptr);

    return new_ptr;
}

void GC_free(RAW_PTR ptr) {
    DEBUG("freeing memory: %p", ptr);

    for (size_t i = 0; i < GC_GLOBAL_HEAP.len; i++) {

        if (ptr == GC_GLOBAL_HEAP.blocks[i]) {
            free(GC_GLOBAL_HEAP.blocks[i]);

            GC_GLOBAL_HEAP.len--;

            memcpy(&GC_GLOBAL_HEAP.blocks[i], &GC_GLOBAL_HEAP.blocks[i + 1], GC_GLOBAL_HEAP.len - i);
        }
    }
}
