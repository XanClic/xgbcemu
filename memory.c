#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbc.h"

// #define DUMP

static void no_ramw_handler(uintptr_t addr, uint8_t value);
static uint8_t no_ramr_handler(uintptr_t addr);
static void no_romw_handler(uintptr_t addr, uint8_t value);
static uint8_t no_romr_handler(uintptr_t addr);
static void no_load_handler(void);

extern void mbc1_ram_write(uintptr_t addr, uint8_t value);
extern void mbc2_ram_write(uintptr_t addr, uint8_t value);
extern void mbc3_ram_write(uintptr_t addr, uint8_t value);
extern void mbc5_ram_write(uintptr_t addr, uint8_t value);
extern uint8_t mbc1_ram_read(uintptr_t addr);
extern uint8_t mbc2_ram_read(uintptr_t addr);
extern uint8_t mbc3_ram_read(uintptr_t addr);
extern uint8_t mbc5_ram_read(uintptr_t addr);
extern void mbc1_rom_write(uintptr_t addr, uint8_t value);
extern void mbc2_rom_write(uintptr_t addr, uint8_t value);
extern void mbc3_rom_write(uintptr_t addr, uint8_t value);
extern void mbc5_rom_write(uintptr_t addr, uint8_t value);
extern uint8_t mbc1_rom_read(uintptr_t addr);
extern uint8_t mbc2_rom_read(uintptr_t addr);
extern uint8_t mbc3_rom_read(uintptr_t addr);
extern uint8_t mbc5_rom_read(uintptr_t addr);
extern void mbc1_load(void);
extern void mbc2_load(void);
extern void mbc3_load(void);
extern void mbc5_load(void);

static void (*const cart_ram_write[6])(uintptr_t addr, uint8_t value) =
{
    &no_ramw_handler,
    &mbc1_ram_write,
    &mbc2_ram_write,
    &mbc3_ram_write,
    &no_ramw_handler,
    &mbc5_ram_write
};

static uint8_t (*const cart_ram_read[6])(uintptr_t addr) =
{
    &no_ramr_handler,
    &mbc1_ram_read,
    &mbc2_ram_read,
    &mbc3_ram_read,
    &no_ramr_handler,
    &mbc5_ram_read
};

static void (*const cart_rom_write[6])(uintptr_t addr, uint8_t value) =
{
    &no_romw_handler,
    &mbc1_rom_write,
    &mbc2_rom_write,
    &mbc3_rom_write,
    &no_romw_handler,
    &mbc5_rom_write
};

static uint8_t (*const cart_rom_read[6])(uintptr_t addr) =
{
    &no_romr_handler,
    &mbc1_rom_read,
    &mbc2_rom_read,
    &mbc3_rom_read,
    &no_romr_handler,
    &mbc5_rom_read
};

static void (*const cart_load[6])(void) =
{
    &no_load_handler,
    &mbc1_load,
    &mbc2_load,
    &mbc3_load,
    &no_load_handler,
    &mbc5_load
};

void init_memory(void)
{
    int_ram = malloc(4096);
    full_int_wram = malloc(32768 - 4096);
    int_wram = full_int_wram;
    oam_io = calloc(1, 512);
    io_regs = (struct io *)&oam_io[0x100];
    full_vidram = calloc(1, 16384);
    vidram = full_vidram;
    vidmem = malloc(256 * 256 * 4);
    memset(vidmem, 255, 256 * 256 * 4);
}

void load_memory(void)
{
    fseek(fp, 0, SEEK_SET);
    if (!mbc)
    {
        base_rom_ptr = malloc(4096);
        fread(base_rom_ptr, 1024, 4, fp);
        if (rom_size > 1)
        {
            rom_bank_ptr = malloc(4096);
            fread(rom_bank_ptr, 1024, 4, fp);
        }
    }
    else
        cart_load[mbc]();
}

void mem_writeb(uintptr_t addr, uint8_t value)
{
    #ifdef DUMP
    printf("0x%02X -> 0x%04X\n", (unsigned)value, (unsigned)addr);
    #endif

    if (((addr >= 0xFF00) && (addr <= 0xFF55)) || ((addr >= 0xFF68) && (addr <= 0xFF70)) || (addr == 0xFFFF))
        io_outb((uint8_t)(addr - 0xFF00), value);
    else if (addr >= 0xFE00)
        oam_io[addr - 0xFE00] = value;
    else if (addr >= 0xC000)
    {
        if (addr & 0x1000)
            int_wram[addr & 0x0FFF] = value;
        else
            int_ram[addr & 0x0FFF] = value;
    }
    else if (addr >= 0xA000)
    {
        if (ext_ram_ptr != NULL)
            ext_ram_ptr[addr - 0xA000] = value;
        else
            cart_ram_write[mbc](addr - 0xA000, value);
    }
    else if (addr >= 0x8000)
    {
        // printf("[VMEM] 0x%02X -> 0x%04X\n", (unsigned)value, (unsigned)addr);
        vidram[addr - 0x8000] = value;
    }
    else
    {
        if (!mbc)
        {
            fprintf(stderr, "No MBC available (tried ROM write 0x%02X to 0x%04X)!\n", value, addr);
            exit(1);
        }
        cart_rom_write[mbc](addr, value);
    }
}

void mem_writew(uintptr_t addr, uint16_t value)
{
    // TODO
    mem_writeb(addr + 0,  value & 0x00FF      );
    mem_writeb(addr + 1, (value & 0xFF00) >> 8);
}

#ifndef DUMP
uint8_t mem_readb(uintptr_t addr)
#else
uint8_t mem_readb_(uintptr_t addr)
#endif
{
    if (addr >= 0xFE00)
        return oam_io[addr - 0xFE00];
    if (addr >= 0xC000)
    {
        if (addr & 0x1000)
            return int_wram[addr & 0x0FFF];
        else
            return int_ram[addr & 0x0FFF];
    }
    if (addr >= 0xA000)
    {
        if (ext_ram_ptr == NULL)
            return cart_ram_read[mbc](addr - 0xA000);
        return ext_ram_ptr[addr - 0xA000];
    }
    if (addr >= 0x8000)
        return vidram[addr - 0x8000];
    if (addr >= 0x4000)
    {
        if (rom_bank_ptr == NULL)
            return cart_rom_read[mbc](addr);
        return rom_bank_ptr[addr - 0x4000];
    }
    if (base_rom_ptr == NULL)
        return cart_rom_read[mbc](addr);
    return base_rom_ptr[addr];
}

#ifdef DUMP
uint8_t mem_readb(uintptr_t addr)
{
    uint8_t ret = mem_readb_(addr);
    printf("0x%04X == 0x%02X\n", (unsigned)addr, (unsigned)ret);
    return ret;
}
#endif

uint16_t mem_readw(uintptr_t addr)
{
    // TODO
    return mem_readb(addr) | (mem_readb(addr + 1) << 8);
}

static void no_ramw_handler(uintptr_t addr, uint8_t value)
{
    fprintf(stderr, "No RAM write handler available! (MBC%i, 0x%02X to 0x%04X)\n", mbc, value, addr);
    exit(1);
}

static uint8_t no_ramr_handler(uintptr_t addr)
{
    fprintf(stderr, "No RAM read handler available! (MBC%i, from 0x%04X)\n", mbc, addr);
    exit(1);
}

static void no_romw_handler(uintptr_t addr, uint8_t value)
{
    fprintf(stderr, "No ROM write handler available! (MBC%i, 0x%02X to 0x%04X)\n", mbc, value, addr);
    exit(1);
}

static uint8_t no_romr_handler(uintptr_t addr)
{
    fprintf(stderr, "No ROM read handler available! (MBC%i, from 0x%04X)\n", mbc, addr);
    exit(1);
}

static void no_load_handler(void)
{
    fprintf(stderr, "No load handler available! (MBC%i)\n", mbc);
    exit(1);
}
