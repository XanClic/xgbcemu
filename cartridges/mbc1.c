#include <stdint.h>
#include <stdlib.h>

#include "gbc.h"

static int current_ram_bank = 0, mode = 0;
static uint8_t *ram_banks[16] = { NULL }, *rom_banks[512] = { NULL };

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
    if (addr < 0x2000)
    {
        if (val == 0x0A)
            ext_ram_ptr = ram_banks[current_ram_bank];
        else
            ext_ram_ptr = NULL;
    }
    else if (addr < 0x4000)
    {
        current_rom_bank = val & 0x1F;
        if (!current_rom_bank)
            current_rom_bank = 1;
        rom_bank_ptr = rom_banks[current_rom_bank];
    }
    else if (addr < 0x6000)
    {
        if (mode)
        {
            current_ram_bank = val & 0x03;
            ext_ram_ptr = ram_banks[current_ram_bank];
        }
        else
        {
            current_rom_bank &= 0x1F;
            current_rom_bank |= (val & 0x03) << 5;
            rom_bank_ptr = rom_banks[current_rom_bank];
        }
    }
    else
        mode = val & 1;
}

uint8_t mbc1_rom_read(uintptr_t addr)
{
    os_eprint("No MBC1 ROM read handler (from 0x%04X)\n", addr);
    exit_err();
}

void mbc1_load(void)
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

        printf("[mbc1] Battery won't be saved automatically, use space to save its content.\n");
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

void mbc1_save(void)
{
    os_file_setpos(save, 0);
    for (int i = 0; i < ram_size; i++)
        os_file_write(save, 8192, ram_banks[i]);

    printf("[mbc1] Battery content written manually.\n");
}
