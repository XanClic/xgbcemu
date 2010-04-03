#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "gbc.h"

// #define DUMP

static int current_ram_bank = 0, current_rom_bank = 1, creg = 0, ram_val = 0, day_cry_set = 0;
static struct tm *cur_tm = NULL;
uint8_t *ram_banks[4] = { NULL }, *rom_banks[128] = { NULL };

void mbc3_ram_write(uintptr_t addr, uint8_t val)
{
    if (!creg)
    {
        printf("[mbc3] RAM should be enabled (bank %i), but write 0x%02X to 0x%04X could not be handled.\n", current_ram_bank, val, addr);
        exit(1);
    }
    if (creg == 0x0C)
        day_cry_set = val & 0x80;
}

uint8_t mbc3_ram_read(uintptr_t addr)
{
    if (!creg)
    {
        printf("[mbc3] RAM should be enabled (bank %i), but read from 0x%04X could not be handled.\n", current_ram_bank, addr);
        exit(1);
    }
    return ram_val;
}

void mbc3_rom_write(uintptr_t addr, uint8_t val)
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
        if (current_rom_bank == (val & 0x7F))
            return;

        current_rom_bank = val & 0x7F;
        if (!current_rom_bank)
            current_rom_bank = 1;

        #ifdef DUMP
        printf("Selected ROM bank %i\n", current_rom_bank);
        #endif

        rom_bank_ptr = rom_banks[current_rom_bank];
    }
    else if (addr < 0x6000)
    {
        int sel = val & 0x0F;
        if (sel < 8)
        {
            sel &= 0x03;

            if (current_ram_bank == sel)
                return;

            current_ram_bank = sel;
            #ifdef DUMP
            printf("Selected RAM bank %i\n", current_ram_bank);
            #endif
            creg = 0;

            ext_ram_ptr = ram_banks[current_ram_bank];
        }
        else if (sel >= 8)
        {
            if (!rtc)
            {
                fprintf(stderr, "[mbc3] Tried to read the RTC without having it!\n");
                exit(1);
            }

            #ifdef DUMP
            printf("[mbc3] Selected clock register %i\n", sel);
            #endif
            creg = sel;

            switch (sel)
            {
                case 8:
                    ram_val = cur_tm->tm_sec;
                    break;
                case 9:
                    ram_val = cur_tm->tm_min;
                    break;
                case 10:
                    ram_val = cur_tm->tm_hour;
                    break;
                case 11:
                    ram_val = cur_tm->tm_yday & 0xFF;
                    break;
                case 12:
                    ram_val = day_cry_set | ((cur_tm->tm_yday & 0x100) >> 8);
            }

            ext_ram_ptr = NULL;
        }
        else
        {
            fprintf(stderr, "[mbc3] Unknown RAM bank or clock register %i selected\n", sel);
            exit(1);
        }
    }
    else
    {
        if (val != 0x01)
            cur_tm = NULL;
        else
        {
            if (cur_tm == NULL)
            {
                time_t curtime;
                curtime = time(NULL);
                cur_tm = gmtime(&curtime);
            }
        }
    }
}

uint8_t mbc3_rom_read(uintptr_t addr)
{
    fprintf(stderr, "[mbc3] Could not handle ROM read from 0x%04X (ROM bank %i)\n", addr, current_rom_bank);
    exit(1);
}

void mbc3_load(void)
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
