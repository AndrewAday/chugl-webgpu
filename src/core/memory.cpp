#include <cstdlib>
#include <cstring>

#include "memory.h"

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