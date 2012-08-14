#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static void (*enter_shell)(void);

void *os_map_file_into_memory(FILE *file, size_t length)
{
    void *mapping = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(file), 0);
    if (mapping == MAP_FAILED)
        return NULL;
    return mapping;
}

void os_get_line(char *buffer, size_t length)
{
    int actlen;

    fgets(buffer, length, stdin);

    actlen = strlen(buffer);
    if (actlen && (buffer[actlen - 1] == '\n'))
        buffer[--actlen] = 0;
    if (actlen && (buffer[actlen - 1] == '\r'))
        buffer[--actlen] = 0;
}

int os_get_integer(void)
{
    char buf[100];
    os_get_line(buf, 100);
    return atoi(buf);
}

static void shell_handler(int signum)
{
    if (signum != SIGINT)
        return;

    enter_shell();
}

void install_shell_handler(void (*callback)(void))
{
    enter_shell = callback;
    sigaction(SIGINT, &(struct sigaction){ .sa_handler = &shell_handler }, NULL);
}
