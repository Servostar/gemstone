//
// Created by servostar on 5/7/24.
//

#ifndef GC_H
#define GC_H

#include <stdlib.h>

#define malloc GC_malloc
#define free GC_free
#define realloc GC_realloc

typedef void* RAW_PTR;

void GC_init(void);

RAW_PTR GC_malloc(size_t bytes);

RAW_PTR GC_realloc(RAW_PTR ptr, size_t bytes);

void GC_free(RAW_PTR ptr);

#endif //GC_H
