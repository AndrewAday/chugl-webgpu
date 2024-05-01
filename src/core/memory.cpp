#include <cstdlib>
#include <cstring>

#include "core/memory.h"

void* reallocate(void* pointer, i64 oldSize, i64 newSize)
{
    // passing size = 0 to realloc will allocate a "minimum-sized"
    // object
    if (newSize == 0) {
        free(pointer); // no-op if pointer is NULL
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1); // out of memory

    // zero out the new memory
    if (newSize > oldSize) {
        memset((u8*)result + oldSize, 0, newSize - oldSize);
    }

    return result;
}

// =================================================================================================
// Arena Allocator Definitions
// =================================================================================================

void Arena::init(Arena* a, u64 cap)
{
    a->base = ALLOCATE_BYTES(u8, cap);
    a->curr = a->base;
    a->cap  = cap;
}

void* Arena::alloc(Arena* a, u64 size)
{
    ASSERT(a->base != NULL); // must be initialized

    u64 currSize = a->curr - a->base;

    // reallocate more memory if needed
    if (currSize + size > a->cap) {
        u64 oldCap = a->cap;
        u64 newCap = GROW_CAPACITY(oldCap);
        a->base    = GROW_ARRAY(u8, a->base, oldCap, newCap);
    }

    void* result = a->curr;

    // advance the current pointer
    a->curr += size;

    return result;
}

void Arena::pop(Arena* a, u64 size)
{
    ASSERT(a->base != NULL);           // must be initialized
    ASSERT(size <= a->curr - a->base); // cannot pop more than allocated

    size = MIN(size, a->curr - a->base);

    // move the current pointer back
    a->curr -= size;

    // zero out the memory
    memset(a->curr, 0, size);
}

void* Arena::get(Arena* a, u64 offset)
{
    return a->base + offset;
}

void Arena::clear(Arena* a)
{
    u64 size = a->curr - a->base;
    Arena::pop(a, size);
}

void Arena::free(Arena* a)
{
    FREE_ARRAY(u8, a->base, a->cap);
    a->base = NULL;
    a->curr = NULL;
    a->cap  = 0;
}