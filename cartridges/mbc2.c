#include <stdint.h>
#include <stdlib.h>

#include "gbc.h"

void mbc2_ram_write(uintptr_t addr, uint8_t val)
{
    os_eprint("No MBC2 RAM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit_err();
}

uint8_t mbc2_ram_read(uintptr_t addr)
{
    os_eprint("No MBC2 RAM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc2_rom_write(uintptr_t addr, uint8_t val)
{
    os_eprint("No MBC2 ROM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit_err();
}

uint8_t mbc2_rom_read(uintptr_t addr)
{
    os_eprint("No MBC2 ROM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc2_load(void)
{
    os_eprint("No MBC2 load handler\n");
    exit_err();
}

void mbc2_save(void)
{
    os_eprint("No MBC2 save handler\n");
    exit_err();
}
