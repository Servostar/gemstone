
#ifndef GEMSTONE_STD_LIB_MEM_H_
#define GEMSTONE_STD_LIB_MEM_H_

#include <def/api.h>

void heap_alloc(u32 len, u8** ptr);

void heap_realloc(u32 len, u8** ptr);

void heap_free(u8* ptr);

void copy(u8* dst, u8* src, u32 len);

void fill(u8* dst, u8 byte, u32 len);

#endif // GEMSTONE_STD_LIB_MEM_H_
