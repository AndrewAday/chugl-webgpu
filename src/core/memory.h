#pragma once

#include "macros.h"

void* reallocate(void* pointer, i64 oldSize, i64 newSize);

#define ALLOCATE_COUNT(type, count)                                            \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define ALLOCATE_BYTES(type, bytes) (type*)reallocate(NULL, 0, (bytes))

#define FREE_ARRAY(type, arrayPtr, oldCap)                                     \
    do {                                                                       \
        reallocate(arrayPtr, (oldCap) * sizeof(type), 0);                      \
        arrayPtr = NULL;                                                       \
    } while (0)

#define FREE(type, ptr)                                                        \
    do {                                                                       \
        reallocate(ptr, sizeof(type), 0);                                      \
        ptr = NULL;                                                            \
    } while (0)