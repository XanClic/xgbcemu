#include <stdint.h>
#include <stdlib.h>

#include "gbc.h"

static int current_ram_bank = 0, current_rom_bank = 1;
static uint8_t *ram_banks[16] = { NULL }, *rom_banks[512] = { NULL };

void mbc5_ram_write(uintptr_t addr, uint8_t val)
{
    os_eprint("No MBC5 RAM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit_err();
}

uint8_t mbc5_ram_read(uintptr_t addr)
{
    os_eprint("No MBC5 RAM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc5_rom_write(uintptr_t addr, uint8_t val)
{
    if (addr < 0x2000)
    {
        if (val == 0x0A)
            ext_ram_ptr = ram_banks[current_ram_bank];
        else
            ext_ram_ptr = NULL;
    }
    if (addr < 0x3000)
    {
        current_rom_bank &= 0x100;
        current_rom_bank |= val;
        rom_bank_ptr = rom_banks[current_rom_bank];
    }
    else if (addr < 0x4000)
    {
        current_rom_bank &= 0x0FF;
        current_rom_bank |= (val & 1) << 8;
        rom_bank_ptr = rom_banks[current_rom_bank];
    }
    else if (addr < 0x6000)
    {
        current_ram_bank = val & 0xF;
        ext_ram_ptr = ram_banks[current_ram_bank];
    }
}

uint8_t mbc5_rom_read(uintptr_t addr)
{
    os_eprint("No MBC5 ROM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc5_load(void)
{
    for (int i = 0; i < ram_size; i++)
        ram_banks[i] = malloc(8192);
    ext_ram_ptr = ram_banks[0];

    for (int i = 0; i < rom_size; i++)
    {
        rom_banks[i] = malloc(16384);
        fread(rom_banks[i], 1024, 16, fp);
    }
    base_rom_ptr = rom_banks[0];
    rom_bank_ptr = rom_banks[1];
}

void mbc5_save(void)
{
    os_eprint("No MBC5 save handler\n");
    exit_err();
}
