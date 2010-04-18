#ifndef OS_IO_H
#define OS_IO_H

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

typedef int file_obj;
#define INVALID_FILE_HANDLE -1

#define os_print printf
#define os_eprint printf
#define os_print_flush printf
#define os_perr(message) printf("%s: errno == %i\n", message, errno)

#define os_open_file_r(path)    open(path, O_RDONLY)
#define os_open_file_rw(path)   open(path, O_RDWR)
#define os_create_file_rw(path) open(path, O_RDWR | O_CREAT, 0)
#define os_close_file(file)     close(file)

#define os_file_read(file, size, buffer)  read(file, buffer, size)
#define os_file_write(file, size, buffer) write(file, buffer, size)
#define os_file_setpos(file, position)    lseek(file, position, SEEK_SET)

#define install_shell_handler(par)

int os_get_integer(void);
void os_get_line(char *buffer, size_t length);

#endif
