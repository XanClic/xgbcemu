#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
