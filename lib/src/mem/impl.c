
#include <mem/api.h>
#include <capi.h>

#if defined(PLATFORM_WINDOWS)

#include <Windows.h>

#define HEAP_API_GLOBAL_FLAGS HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS

void heapAlloc(u32 len, u8** ptr) {
    HANDLE heap = GetProcessHeap();
    *ptr = HeapAlloc(heap, HEAP_API_GLOBAL_FLAGS, len);
}

void heapRealloc(u32 len, u8** ptr) {
    HANDLE heap = GetProcessHeap();
    *ptr = HeapReAlloc(heap, HEAP_API_GLOBAL_FLAGS, *ptr, len);
}

void heapFree(u8* ptr) {
    HANDLE heap = GetProcessHeap();
    HeapFree(heap, ptr);
}

#elif defined(PLATFORM_POSIX)

#include <malloc.h>

void heapAlloc(u32 len, u8** ptr) {
    *ptr = malloc(len);
}

void heapRealloc(u32 len, u8** ptr) {
    *ptr = realloc(*ptr, len);
}

void heapFree(u8* ptr) {
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