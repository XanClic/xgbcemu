#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void os_get_line(char *buf, size_t size)
{
    int i = 0, ret = 0;

    while (size)
    {
        ret = (int)read(0, &buf[i], 1);
        if (ret && (buf[i] == '\n'))
        {
            buf[i] = 0;
            break;
        }

        if (ret && (buf[i] == '\b'))
        {
            buf[i--] = 0;
            while ((buf[i] & 0xC0) == 0x80)
                buf[i--] = 0;
            buf[i] = 0;
        }
        else
        {
            size -= ret;
            i += ret;
        }
    }
}

int os_get_integer(void)
{
    char buf[100];
    os_get_line(buf, 100);
    return atoi(buf);
}
