#include <stdint.h>
#include <stdlib.h>

#include "gbc.h"

void mbc5_ram_write(uintptr_t addr, uint8_t val)
{
    fprintf(stderr, "No MBC5 RAM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit(1);
}

uint8_t mbc5_ram_read(uintptr_t addr)
{
    fprintf(stderr, "No MBC5 RAM read handler (from 0x%04X)\n", addr);
    exit(1);
}

void mbc5_rom_write(uintptr_t addr, uint8_t val)
{
    fprintf(stderr, "No MBC5 ROM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit(1);
}

uint8_t mbc5_rom_read(uintptr_t addr)
{
    fprintf(stderr, "No MBC5 ROM read handler (from 0x%04X)\n", addr);
    exit(1);
}

void mbc5_load(void)
{
    fprintf(stderr, "No MBC5 load handler\n");
    exit(1);
}
