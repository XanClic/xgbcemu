#ifndef OS_IO_H
#define OS_IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

typedef HANDLE file_obj;
#define INVALID_FILE_HANDLE INVALID_HANDLE_VALUE

#define os_init_console freopen("stdout", "w", stdout); freopen("stderr", "w", stderr);

#define os_print printf
#define os_eprint(...) fprintf(stderr, __VA_ARGS__)
#define os_print_flush(...) printf(__VA_ARGS__), fflush(stdout)
#define os_perr(message) perror(message)

#define os_open_file_r(path)    CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)
#define os_open_file_rw(path)   CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)
#define os_create_file_rw(path) CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL)
#define os_close_file(file)     CloseHandle(file)

static uint32_t winapi_ignore __attribute__((unused));
#define os_file_read(file, size, buffer)  ReadFile(file, buffer, size, (PDWORD)&winapi_ignore, NULL)
#define os_file_write(file, size, buffer) WriteFile(file, buffer, size, (PDWORD)&winapi_ignore, NULL)
#define os_file_setpos(file, position)    SetFilePointer(file, position, NULL, FILE_BEGIN)
#define os_resize_file(file, new_size)    SetFilePointer(file, new_size, NULL, FILE_BEGIN), SetEndOfFile(file)

#define install_shell_handler(cb)

int os_get_integer(void);
void os_get_line(char *buffer, size_t length);
void *os_map_file_into_memory(FILE *file, size_t length);

#endif
