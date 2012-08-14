#include <stdint.h>

#include "gbc.h"

// #define DUMP

static int current_ram_bank = 0;
static uint8_t *ram_banks[16] = { NULL }, *rom_banks[512] = { NULL };

void mbc5_ram_write(uint16_t addr, uint8_t val)
{
    os_eprint("No MBC5 RAM write handler (0x%02X to 0x%04X)\n", val, addr);
    exit_err();
}

uint8_t mbc5_ram_read(uint16_t addr)
{
    os_eprint("No MBC5 RAM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc5_rom_write(uint16_t addr, uint8_t val)
{
    if (addr < 0x2000)
    {
        if (val == 0x0A)
            ext_ram_ptr = ram_banks[current_ram_bank];
        else
            ext_ram_ptr = NULL;
    }
    else if (addr < 0x3000)
    {
        current_rom_bank &= 0x100;
        current_rom_bank |= val;
        rom_bank_ptr = rom_banks[current_rom_bank];
        #ifdef DUMP
        os_print("[mbc5] Switched to ROM bank %i\n", current_rom_bank);
        #endif
    }
    else if (addr < 0x4000)
    {
        current_rom_bank &= 0x0FF;
        current_rom_bank |= (val & 1) << 8;
        rom_bank_ptr = rom_banks[current_rom_bank];
        #ifdef DUMP
        os_print("[mbc5] Switched to ROM bank %i\n", current_rom_bank);
        #endif
    }
    else if (addr < 0x6000)
    {
        current_ram_bank = val & 0xF;
        ext_ram_ptr = ram_banks[current_ram_bank];
    }
    #ifdef DUMP
    else
        os_print("[mbc5] Unhandled ROM write 0x%02X to 0x%04X\n", val, addr);
    #endif
}

uint8_t mbc5_rom_read(uint16_t addr)
{
    os_eprint("No MBC5 ROM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc5_load(void)
{
    uint8_t *base = NULL;
    #ifdef MAP_BATTERY
    base = os_map_file_into_memory(save, ram_size * 8192);
    if (base != NULL)
        for (int i = 0; i < ram_size; i++)
            ram_banks[i] = base + i * 8192;
    #endif
    if (base == NULL)
    {
        os_file_setpos(save, 0);
        for (int i = 0; i < ram_size; i++)
        {
            ram_banks[i] = alloc_cmem(8192);
            os_file_read(save, 8192, ram_banks[i]);
        }

        printf("[mbc5] Battery won't be saved automatically, use space to save its content.\n");
    }
    ext_ram_ptr = ram_banks[0];

    for (int i = 0; i < rom_size; i++)
    {
        rom_banks[i] = alloc_cmem(16384);
        os_file_read(fp, 16384, rom_banks[i]);
    }
    base_rom_ptr = rom_banks[0];
    rom_bank_ptr = rom_banks[1];
}

void mbc5_save(void)
{
    os_file_setpos(save, 0);
    for (int i = 0; i < ram_size; i++)
        os_file_write(save, 8192, ram_banks[i]);

    printf("[mbc5] Battery content written manually.\n");
}
