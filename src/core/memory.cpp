#pragma once

#include <cstdlib>
#include <cstring>

#include "core/macros.h"

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

void* reallocate(void* pointer, i64 oldSize, i64 newSize)
{
    UNUSED_VAR(oldSize);

    // passing size = 0 to realloc will allocate a "minimum-sized"
    // object
    if (newSize == 0) {
        free(pointer); // no-op if pointer is NULL
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1); // out of memory

    // zero out the new memory
    if (oldSize == 0) memset(result, 0, newSize);

    return result;
}