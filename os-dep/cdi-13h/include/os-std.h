#ifndef OS_STD_H
#define OS_STD_H

#include <stdlib.h>

void custom_exit(int) __attribute__((noreturn));

#define exit_err() custom_exit(1)
#define exit_ok()  custom_exit(0)

#define alloc_mem(size)  malloc(size)
#define alloc_cmem(size) calloc(1, size)

#endif
