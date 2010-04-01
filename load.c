#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbc.h"

static const unsigned char id[0x30] =
{
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

void load_rom(const char *fname)
{
    FILE *fp;

    fp = fopen(fname, "r");
    if (fp == NULL)
        exit(1);

    memory = malloc(65536);
    io_regs = (struct io *)&memory[0xFF00];
    fread(memory, 32, 1024, fp);

    fclose(fp);

    if (memcmp(id, memory + 0x104, sizeof(id)))
    {
        printf("Bad ROM!\n");
        exit(1);
    }

    printf("Loading \"");
    for (int i = 0x134; i < 0x143; i++)
    {
        if (!memory[i])
            break;
        printf("%c", memory[i]);
    }
    printf("\", %s...\n", (memory[0x143] & 0x80) ? "GBC" : "GB");

    card_type = memory[0x147];
    rom_size = memory[0x148];
    ram_size = memory[0x149];

    switch (rom_size)
    {
        case 0x52:
            rom_size = 72;
            break;
        case 0x53:
            rom_size = 80;
            break;
        case 0x54:
            rom_size = 96;
            break;
        default:
            if (rom_size > 6)
            {
                printf("Invalid ROM size 0x%02X.\n", rom_size);
                exit(1);
            }
            rom_size = 2 << rom_size;
    }

    switch (ram_size)
    {
        case 0:
            ram_size = 0;
            break;
        case 1:
        case 2:
            ram_size = 1;
            break;
        case 3:
            ram_size = 4;
            break;
        case 4:
            ram_size = 16;
            break;
        default:
            printf("Invalid RAM size 0x%02X.\n", ram_size);
            exit(1);
    }

    printf("%i ROM banks, %i RAM banks.\n", rom_size, ram_size);

    vidmem = malloc(256 * 256 * 4);
    memset(vidmem, 255, 256 * 256 * 4);

    run();

    free(memory);

    return;
}
