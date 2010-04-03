#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbc.h"

#define CART_TYPE(id, mbctype, extram, battery, timer, rumble) \
    case id: \
        mbc = mbctype; \
        ext_ram = extram; \
        batt = battery; \
        rtc = timer; \
        rmbl = rumble; \
        break;

static const unsigned char id[0x30] =
{
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

void load_rom(const char *fname)
{
    uint8_t *start_of_rom;
    int cart_type;

    init_memory();

    fp = fopen(fname, "r");
    if (fp == NULL)
    {
        perror("Couldn't read file");
        exit(1);
    }

    start_of_rom = malloc(0x150);
    if (start_of_rom == NULL)
    {
        perror("Couldn't allocate memory");
        exit(1);
    }

    fread(start_of_rom, 1, 0x150, fp);

    if (memcmp(id, start_of_rom + 0x104, sizeof(id)))
    {
        fprintf(stderr, "Bad ROM!\n");
        exit(1);
    }

    printf("Loading \"");
    for (int i = 0x134; i < 0x143; i++)
    {
        if (!start_of_rom[i])
            break;
        printf("%c", start_of_rom[i]);
    }
    printf("\", %s...\n", (start_of_rom[0x143] & 0x80) ? "GBC" : "GB");

    cart_type = start_of_rom[0x147];
    rom_size = start_of_rom[0x148];
    ram_size = start_of_rom[0x149];

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
                fprintf(stderr, "Invalid ROM size 0x%02X.\n", rom_size);
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
            fprintf(stderr, "Invalid RAM size 0x%02X.\n", ram_size);
            exit(1);
    }

    printf("%i ROM banks, %i RAM banks.\n", rom_size, ram_size);

    switch (cart_type)
    {
        CART_TYPE(0x00, 0, 0, 0, 0, 0);
        CART_TYPE(0x01, 1, 0, 0, 0, 0);
        CART_TYPE(0x02, 1, 1, 0, 0, 0);
        CART_TYPE(0x03, 1, 1, 1, 0, 0);
        CART_TYPE(0x05, 2, 0, 0, 0, 0);
        CART_TYPE(0x06, 2, 0, 1, 0, 0);
        CART_TYPE(0x08, 0, 1, 0, 0, 0);
        CART_TYPE(0x09, 0, 1, 1, 0, 0);
        CART_TYPE(0x0F, 3, 0, 1, 1, 0);
        CART_TYPE(0x10, 3, 1, 1, 1, 0);
        CART_TYPE(0x11, 3, 0, 0, 0, 0);
        CART_TYPE(0x12, 3, 1, 0, 0, 0);
        CART_TYPE(0x13, 3, 1, 1, 0, 0);
        CART_TYPE(0x19, 5, 0, 0, 0, 0);
        CART_TYPE(0x1A, 5, 1, 0, 0, 0);
        CART_TYPE(0x1B, 5, 1, 1, 0, 0);
        CART_TYPE(0x1C, 5, 0, 0, 0, 1);
        CART_TYPE(0x1D, 5, 1, 0, 0, 1);
        CART_TYPE(0x1E, 5, 1, 1, 0, 1);
        default:
            fprintf(stderr, "Unknown cartridge type 0x%02X.\n", cart_type);
            exit(1);
    }

    printf("Cartridge type: ROM");
    if (mbc)
        printf("+MBC%i", mbc);
    if (ext_ram)
        printf("+RAM");
    if (batt)
        printf("+BATT");
    if (rtc)
        printf("+TIMER");
    if (rmbl)
        printf("+RUMBLE");
    printf("\n");

    load_memory();

    run();
}
