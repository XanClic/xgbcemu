#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>

void *os_map_file_into_memory(FILE *file, size_t length)
{
    void *mapping = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(file), 0);
    if (mapping == MAP_FAILED)
        return NULL;
    return mapping;
}
