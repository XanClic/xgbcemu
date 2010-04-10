#ifndef OS_STD_H
#define OS_STD_H

#include <stdlib.h>

#define exit_err() exit(1)
#define exit_ok()  exit(0)

#define alloc_mem(size)  malloc(size)
#define alloc_cmem(size) calloc(1, size)
#define free_mem(mem)    free(mem)

#endif
