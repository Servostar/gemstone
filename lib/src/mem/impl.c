
#include <mem/api.h>

#if defined(_WIN32) || defined (_WIN64)

#include <Windows.h>

#define HEAP_API_GLOBAL_FLAGS HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS

void heap_alloc(u32 len, u8** ptr) {
    HANDLE heap = GetProcessHeap();
    *ptr = HeapAlloc(heap, HEAP_API_GLOBAL_FLAGS, len);
}

void heap_realloc(u32 len, u8** ptr) {
    HANDLE heap = GetProcessHeap();
    *ptr = HeapReAlloc(heap, HEAP_API_GLOBAL_FLAGS, *ptr, len);
}

void heap_free(u8* ptr) {
    HANDLE heap = GetProcessHeap();
    HeapFree(heap, ptr);
}

#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__)

#include <malloc.h>

void heap_alloc(u32 len, u8** ptr) {
    *ptr = malloc(len);
}

void heap_realloc(u32 len, u8** ptr) {
    *ptr = realloc(*ptr, len);
}

void heap_free(u8* ptr) {
    free(ptr);
}

#endif

#include <string.h>

void copy(u8* dst, u8* src, u32 len) {
    memcpy(dst, src, len);
}

void fill(u8* dst, u8 byte, u32 len) {
    memset(dst, byte, len);
}