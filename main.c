#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbc.h"

int main(int argc, char *argv[])
{
    if ((argc != 2) || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))
    {
        fprintf(stderr, "Usage: gxemu <ROM>\n");
        return EXIT_FAILURE;
    }

    load_rom(argv[1]);

    return EXIT_SUCCESS;
}
