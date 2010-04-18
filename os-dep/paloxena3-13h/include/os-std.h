#ifndef OS_STD_H
#define OS_STD_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void custom_exit(int) __attribute__((noreturn));

#define exit_err() custom_exit(1)
#define exit_ok()  custom_exit(0)

#define alloc_mem(size)  malloc(size)
static inline void *alloc_cmem(size_t size)
{
    void *mem = malloc(size);
    if (mem == NULL)
        return NULL;
    memset(mem, 0, size);
    return mem;
}

#endif
