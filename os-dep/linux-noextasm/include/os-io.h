#ifndef OS_IO_H
#define OS_IO_H

#include <stddef.h>
#include <stdio.h>

typedef FILE *file_obj;

#define os_print printf
#define os_eprint(...) fprintf(stderr, __VA_ARGS__)
#define os_perr(message) perror(message)

#define os_open_file_r(path)    fopen(path, "r")
#define os_open_file_rw(path)   fopen(path, "r+")
#define os_create_file_rw(path) fopen(path, "w+")
#define os_close_file(file)     fclose(file)

#define os_file_read(file, size, buffer)  fread(buffer, 1, size, file)
#define os_file_write(file, size, buffer) fwrite(buffer, 1, size, file)
#define os_file_setpos(file, position)    fseek(file, position, SEEK_SET)

void *os_map_file_into_memory(FILE *file, size_t length);

#endif
