#include <stdint.h>
#include <stdlib.h>

#include "gbc.h"

void mbc2_ram_write(uintptr_t addr, uint8_t val)
{
    fprintf(stderr, "No MBC2 RAM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit(1);
}

uint8_t mbc2_ram_read(uintptr_t addr)
{
    fprintf(stderr, "No MBC2 RAM read handler (from 0x%04X)\n", addr);
    exit(1);
}

void mbc2_rom_write(uintptr_t addr, uint8_t val)
{
    fprintf(stderr, "No MBC2 ROM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit(1);
}

uint8_t mbc2_rom_read(uintptr_t addr)
{
    fprintf(stderr, "No MBC2 ROM read handler (from 0x%04X)\n", addr);
    exit(1);
}

void mbc2_load(void)
{
    fprintf(stderr, "No MBC2 load handler\n");
    exit(1);
}
