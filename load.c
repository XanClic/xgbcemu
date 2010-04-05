#include <stdint.h>
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

void load_rom(const char *fname, const char *sname)
{
    uint8_t *start_of_rom;
    int cart_type, save_created = 0;

    init_memory();

    fp = os_open_file_r(fname);
    if (fp == NULL)
    {
        os_perr("Couldn't read ROM file");
        exit_err();
    }

    save = os_open_file_rw(sname);
    if (save == NULL)
    {
        save = os_create_file_rw(sname);
        save_created = 1;
        if (save == NULL)
        {
            os_close_file(fp);
            os_perr("Couldn't create save file");
            exit_err();
        }
    }

    start_of_rom = alloc_mem(0x150);
    if (start_of_rom == NULL)
    {
        os_perr("Couldn't allocate memory");
        exit_err();
    }

    os_file_read(fp, 0x150, start_of_rom);

    if (memcmp(id, start_of_rom + 0x104, sizeof(id)))
    {
        os_eprint("Bad ROM!\n");
        exit_err();
    }

    os_print("Loading \"");
    for (int i = 0x134; i < 0x143; i++)
    {
        if (!start_of_rom[i])
            break;
        os_print("%c", start_of_rom[i]);
    }
    os_print("\", %s...\n", (start_of_rom[0x143] & 0x80) ? "GBC" : "GB");

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
                os_eprint("Invalid ROM size 0x%02X.\n", rom_size);
                exit_err();
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
            os_eprint("Invalid RAM size 0x%02X.\n", ram_size);
            exit_err();
    }

    os_print("%i ROM banks, %i RAM banks.\n", rom_size, ram_size);

    os_resize_file(save, ram_size * 8192);

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
            os_eprint("Unknown cartridge type 0x%02X.\n", cart_type);
            exit_err();
    }

    os_print("Cartridge type: ROM");
    if (mbc)
        os_print("+MBC%i", mbc);
    if (ext_ram)
        os_print("+RAM");
    if (batt)
        os_print("+BATT");
    if (rtc)
        os_print("+TIMER");
    if (rmbl)
        os_print("+RUMBLE");
    os_print("\n");

    load_memory();

    run();
}
