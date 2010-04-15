#include <stdint.h>
#include <string.h>

#include "gbc.h"

// #define DUMP
// #define DUMP_IO
// #define DUMP_VID_WRITES

static void no_ramw_handler(uintptr_t addr, uint8_t value);
static uint8_t no_ramr_handler(uintptr_t addr);
static void no_romw_handler(uintptr_t addr, uint8_t value);
static uint8_t no_romr_handler(uintptr_t addr);
static void no_load_handler(void);
static void no_save_handler(void);

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
extern void mbc1_save(void);
extern void mbc2_save(void);
extern void mbc3_save(void);
extern void mbc5_save(void);

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

static void (*const cart_save[6])(void) =
{
    &no_save_handler,
    &mbc1_save,
    &mbc2_save,
    &mbc3_save,
    &no_save_handler,
    &mbc5_save
};

void init_memory(void)
{
    int_ram = alloc_mem(4096);
    full_int_wram = alloc_mem(32768 - 4096);
    int_wram = full_int_wram;
    oam_io = alloc_cmem(512);
    io_regs = (struct io *)&oam_io[0x100];
    full_vidram = alloc_cmem(16384);
    vidram = full_vidram;
    vidmem = alloc_mem(256 * 256 * 4);
    memset(vidmem, 255, 256 * 256 * 4);
}

void load_memory(void)
{
    os_file_setpos(fp, 0);
    if (!mbc)
    {
        base_rom_ptr = alloc_mem(4096);
        os_file_read(fp, 4096, base_rom_ptr);
        if (rom_size > 1)
        {
            rom_bank_ptr = alloc_mem(4096);
            os_file_read(fp, 4096, rom_bank_ptr);
        }
    }
    else
        cart_load[mbc]();
}

void save_to_disk(void)
{
    cart_save[mbc]();
}

void mem_writeb(uintptr_t addr, uint8_t value)
{
    #ifdef DUMP
    os_print("0x%02X -> 0x%04X\n", (unsigned)value, (unsigned)addr);
    #endif

    if ((addr >= 0x8000) && (addr < 0xFE00))
    {
        if (addr >= 0xC000)
        {
            if (addr & 0x1000)
                int_wram[addr & 0x0FFF] = value;
            else
                int_ram[addr & 0x0FFF] = value;
        }
        else if (addr < 0xA000)
        {
            #ifdef DUMP_VID_WRITES
            os_print("0x%02X -> 0x%04X\n", (unsigned)value, (unsigned)addr);
            #endif
            vidram[addr - 0x8000] = value;
        }
        else
        {
            if (ext_ram_ptr != NULL)
                ext_ram_ptr[addr - 0xA000] = value;
            else
                cart_ram_write[mbc](addr - 0xA000, value);
        }
    }
    else if (((addr >= 0xFF00) && (addr <= 0xFF55)) || ((addr >= 0xFF68) && (addr <= 0xFF70)) || (addr == 0xFFFF))
    {
        #ifdef DUMP_IO
        os_print("[i/o] 0x%02X -> 0x%04X\n", value, addr);
        #endif
        io_outb((uint8_t)(addr - 0xFF00), value);
    }
    else if (addr >= 0xFE00)
        oam_io[addr - 0xFE00] = value;
    else
    {
        if (!mbc)
        {
            os_eprint("No MBC available (tried ROM write 0x%02X to 0x%04X)!\n", value, addr);
            exit_err();
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
    if (addr < 0x8000)
    {
        if (addr < 0x4000)
        {
            if (base_rom_ptr == NULL)
                return cart_rom_read[mbc](addr);
            return base_rom_ptr[addr];
        }
        if (rom_bank_ptr == NULL)
            return cart_rom_read[mbc](addr);
        return rom_bank_ptr[addr - 0x4000];
    }
    else if (addr >= 0xFE00)
    {
        #ifdef DUMP_IO
        if (((addr >= 0xFF00) && (addr <= 0xFF55)) || ((addr >= 0xFF68) && (addr <= 0xFF70)) || (addr == 0xFFFF))
            os_print("[i/o] 0x%04X -> 0x%02X\n", addr, oam_io[addr - 0xFE00]);
        #endif
        return oam_io[addr - 0xFE00];
    }
    else if (addr >= 0xC000)
    {
        if (addr & 0x1000)
            return int_wram[addr & 0x0FFF];
        return int_ram[addr & 0x0FFF];
    }
    else if (addr < 0xA000)
        return vidram[addr - 0x8000];
    else
    {
        if (ext_ram_ptr == NULL)
            return cart_ram_read[mbc](addr - 0xA000);
        return ext_ram_ptr[addr - 0xA000];
    }
}

#ifdef DUMP
uint8_t mem_readb(uintptr_t addr)
{
    uint8_t ret = mem_readb_(addr);
    os_print("0x%04X == 0x%02X\n", (unsigned)addr, (unsigned)ret);
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
    os_eprint("No RAM write handler available! (MBC%i, 0x%02X to 0x%04X)\n", mbc, value, addr);
    exit_err();
}

static uint8_t no_ramr_handler(uintptr_t addr)
{
    os_eprint("No RAM read handler available! (MBC%i, from 0x%04X)\n", mbc, addr);
    exit_err();
}

static void no_romw_handler(uintptr_t addr, uint8_t value)
{
    addr = value;
}

static uint8_t no_romr_handler(uintptr_t addr)
{
    os_eprint("No ROM read handler available! (MBC%i, from 0x%04X)\n", mbc, addr);
    exit_err();
}

static void no_load_handler(void)
{
    os_eprint("No load handler available! (MBC%i)\n", mbc);
    exit_err();
}

static void no_save_handler(void)
{
    os_eprint("No save handler available! (MBC%i)\n", mbc);
    exit_err();
}
