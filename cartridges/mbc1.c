#include <stdint.h>
#include <stdlib.h>

#include "gbc.h"

void mbc1_ram_write(uintptr_t addr, uint8_t val)
{
    os_eprint("No MBC1 RAM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit_err();
}

uint8_t mbc1_ram_read(uintptr_t addr)
{
    os_eprint("No MBC1 RAM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc1_rom_write(uintptr_t addr, uint8_t val)
{
    os_eprint("No MBC1 ROM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit_err();
}

uint8_t mbc1_rom_read(uintptr_t addr)
{
    os_eprint("No MBC1 ROM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc1_load(void)
{
    os_eprint("No MBC1 load handler\n");
    exit_err();
}

void mbc1_save(void)
{
    os_eprint("No MBC1 save handler\n");
    exit_err();
}
