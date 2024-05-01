#pragma once
#include "core/macros.h"

void* reallocate(void* pointer, i64 oldSize, i64 newSize);

#define ALLOCATE_TYPE(type) (type*)reallocate(NULL, 0, sizeof(type))

#define ALLOCATE_COUNT(type, count)                                            \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define ALLOCATE_BYTES(type, bytes) (type*)reallocate(NULL, 0, (bytes))

#define GROW_CAPACITY(cap) ((cap) < 8 ? 8 : (cap) * 2)
#define GROW_ARRAY(type, arrayPtr, oldCap, newCap)                             \
    (type*)reallocate(arrayPtr, (oldCap) * sizeof(type),                       \
                      (newCap) * sizeof(type))

#define FREE_ARRAY(type, arrayPtr, oldCap)                                     \
    do {                                                                       \
        reallocate(arrayPtr, (oldCap) * sizeof(type), 0);                      \
        arrayPtr = NULL;                                                       \
    } while (0)

#define FREE_TYPE(type, ptr)                                                   \
    do {                                                                       \
        reallocate(ptr, sizeof(type), 0);                                      \
        ptr = NULL;                                                            \
    } while (0)

#define FREE(ptr)                                                              \
    do {                                                                       \
        reallocate(ptr, 0, 0);                                                 \
        ptr = NULL;                                                            \
    } while (0)

// ============================================================================
// Arena Allocator
// ============================================================================

struct Arena {
    u8* base;
    u8* curr;
    u64 cap;

    static void init(Arena* a, u64 cap);
    static void* alloc(Arena* a, u64 size);
    static void pop(Arena* a, u64 size);
    static void* get(Arena* a, u64 offset);
    static void clear(Arena* a);
    static void free(Arena* a);
};