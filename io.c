#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gbc.h"

static void int_flag(uint8_t value)
{
    io_regs->int_flag = value;
    generate_interrupts();
}

static void int_enable(uint8_t value)
{
    io_regs->int_enable = value;
    generate_interrupts();
}

static void nop(uint8_t value, int offset)
{
    oam_io[0x100 + offset] = value;
}

static void lcdc(uint8_t value)
{
    io_regs->lcdc = value;

    if (value & (1 << 7))
        lcd_on = 1;
    else
        lcd_on = 0;

    if (value & (1 << 6))
        wtm = (uint8_t *)&vidram[0x1C00];
    else
        wtm = (uint8_t *)&vidram[0x1800];

    if (value & (1 << 4))
    {
        bwtd[0] = (uint8_t *)&full_vidram[0x0000];
        bwtd[1] = (uint8_t *)&full_vidram[0x2000];
    }
    else
    {
        bwtd[0] = (uint8_t *)&full_vidram[0x1000];
        bwtd[1] = (uint8_t *)&full_vidram[0x3000];
    }

    if (value & (1 << 3))
    {
        btm[0] = (uint8_t *)&full_vidram[0x1C00];
        btm[1] = (uint8_t *)&full_vidram[0x3C00];
    }
    else
    {
        btm[0] = (uint8_t *)&full_vidram[0x1800];
        btm[1] = (uint8_t *)&full_vidram[0x3800];
    }
}

static void p1(uint8_t value)
{
    value &= KEY_DIR | KEY_OTH;
    if (!(value & KEY_DIR))
        value |= ~((keystates & 0xF0) >> 4);
    if (!(value & KEY_OTH))
        value |= ~(keystates & 0x0F);

    io_regs->p1 = value;
}

static void dma(uint8_t value)
{
    int addr = value << 8;

    if (value > 0xF1)
        return;

    for (int i = 0; i < 160; i++)
        mem_writeb(0xFE00 + i, mem_readb(addr + i));
}

static void stat(uint8_t value)
{
    io_regs->stat &= 7;
    io_regs->stat |= value & ~7;
}

static void divreg(void)
{
    io_regs->div = 0;
}

static void svbk(uint8_t value)
{
    int new_bank = value & 0x07;
    if (!new_bank)
        new_bank = 1;

    io_regs->svbk = new_bank;

    int_wram = &full_int_wram[(new_bank - 1) * 4096];
}

static void vbk(uint8_t value)
{
    value &= 1;
    io_regs->svbk = value;
    vidram = &full_vidram[value * 8192];
}

/**
 * Background palette index.
 *
 * Writing to this register selects the palette index to be used for operations
 * on the BCPD register.
 *
 * Bit  0    : If set, use the MSB instead of the LSB.
 * Bits 1 – 5: Palette index
 * Bit  7    : If set, increment the index after each write to BCPD.
 */
static void bcps(uint8_t value)
{
    int pval = bpalette[(value >> 1) & 0x1F];
    int high = value & 1;

    io_regs->bcps = value & 0xBF;
    io_regs->bcpd = high ? (pval >> 8) : (pval & 0xFF);
}

/**
 * Background palette data.
 *
 * Reading from this register returns the palette index' data selected by BCPS.
 * Writing to it changes that data.
 *
 * Bits 0 – 7: New value
 */
static void bcpd(uint8_t value)
{
    int i = (io_regs->bcps >> 1) & 0x1F;
    int high = io_regs->bcps & 1;

    io_regs->bcpd = value;

    if (high)
    {
        // Change high byte (MSb must be reset)
        bpalette[i] &= 0x00FF;
        bpalette[i] |= (value << 8) & 0x7F00;
    }
    else
    {
        // Change low byte
        bpalette[i] &= 0xFF00;
        bpalette[i] |= value;
    }

    // Use auto-increment mode
    if (io_regs->bcps & 0x80)
        bcps(io_regs->bcps + 1);
}

/**
 * Object palette index.
 *
 * See BCPS for more information.
 */
static void ocps(uint8_t value)
{
    int pval = opalette[(value >> 1) & 0x1F];
    int high = value & 1;

    io_regs->ocps = value & 0xBF;
    io_regs->ocpd = high ? (pval >> 8) : (pval & 0xFF);
}

/**
 * Object palette data.
 *
 * See BCPD for more information.
 */
static void ocpd(uint8_t value)
{
    int i = (io_regs->ocps >> 1) & 0x1F;
    int high = io_regs->ocps & 1;

    io_regs->ocpd = value;

    if (high)
    {
        // Change high byte (MSB must be reset)
        opalette[i] &= 0x00FF;
        opalette[i] |= (value << 8) & 0x7F00;
    }
    else
    {
        // Change low byte
        opalette[i] &= 0xFF00;
        opalette[i] |= value;
    }

    // Use auto-increment mode
    if (io_regs->ocps & 0x80)
        ocps(io_regs->ocps + 1);
}

static uint16_t hdma_src = 0, hdma_dest = 0;

static void hdma1(uint8_t val)
{
    if ((val >= 0x80) && (val < 0xA0))
        val = 0;
    if (val >= 0xE0)
        val -= 0x20;

    hdma_src &= 0x00F0;
    hdma_src |= (val << 8);

    io_regs->hdma1 = val;
}

static void hdma2(uint8_t val)
{
    hdma_src &= 0xFF00;
    hdma_src |= val & 0xF0;

    io_regs->hdma2 = val;
}

static void hdma3(uint8_t val)
{
    val &= 0x1F;
    val |= 0x80;

    hdma_dest &= 0x00F0;
    hdma_dest |= (val << 8);

    io_regs->hdma3 = val;
}

static void hdma4(uint8_t val)
{
    hdma_dest &= 0xFF00;
    hdma_dest |= val & 0xF0;

    io_regs->hdma4 = val;
}

void hdma_copy_16b(void)
{
    if (hdma_src < 0x4000)
        memcpy(&vidram[hdma_dest - 0x8000], &base_rom_ptr[hdma_src], 16);
    else if (hdma_src < 0x8000)
        memcpy(&vidram[hdma_dest - 0x8000], &rom_bank_ptr[hdma_src - 0x4000], 16);
    else if (hdma_src < 0xC000)
        memcpy(&vidram[hdma_dest - 0x8000], &ext_ram_ptr[hdma_src - 0xA000], 16);
    else if (hdma_src < 0xD000)
        memcpy(&vidram[hdma_dest - 0x8000], &int_ram[hdma_src - 0xC000], 16);
    else
        memcpy(&vidram[hdma_dest - 0x8000], &int_wram[hdma_src - 0xD000], 16);

    hdma_src += 16;
    hdma_dest += 16;

    io_regs->hdma1 = hdma_src >> 8;
    io_regs->hdma2 = hdma_src & 0xFF;
    io_regs->hdma3 = hdma_dest >> 8;
    io_regs->hdma4 = hdma_dest & 0xFF;
    if (--io_regs->hdma5 & 0x80)
        hdma_on = 0;

    update_timer(8);
    if (hdma_on)
        generate_interrupts();
}

static void hdma5(uint8_t val)
{
    if (hdma_on)
    {
        if (val & 0x80)
            io_regs->hdma5 = val & 0x7F;
        else
        {
            io_regs->hdma5 = 0xFF;
            hdma_on = 0;
        }
        return;
    }

    io_regs->hdma5 = val & 0x7F;

    if (val & 0x80)
    {
        hdma_on = 1;
        return;
    }

    while (!(io_regs->hdma5 & 0x80))
        hdma_copy_16b();

    generate_interrupts();
}

static void key1(uint8_t val)
{
    io_regs->key1 &= 0x80;
    io_regs->key1 |= val & 1;
}

static void (*const io_handlers[256])(uint8_t value) =
{
    &p1, // p1
    (void (*)(uint8_t))&nop, // sb
    (void (*)(uint8_t))&nop, // sc
    NULL, // rsvd1
    (void (*)(uint8_t))&divreg, // div
    (void (*)(uint8_t))&nop, // tima
    (void (*)(uint8_t))&nop, // tma
    (void (*)(uint8_t))&nop, // tac
    NULL, // rsvd2
    NULL, // rsvd2
    NULL, // rsvd2
    NULL, // rsvd2
    NULL, // rsvd2
    NULL, // rsvd2
    NULL, // rsvd2
    &int_flag, // int_flag
    (void (*)(uint8_t))&nop, // nr10
    (void (*)(uint8_t))&nop, // nr11
    (void (*)(uint8_t))&nop, // nr12
    (void (*)(uint8_t))&nop, // nr13
    (void (*)(uint8_t))&nop, // nr14
    (void (*)(uint8_t))&nop, // rsvd3
    (void (*)(uint8_t))&nop, // nr21
    (void (*)(uint8_t))&nop, // nr22
    (void (*)(uint8_t))&nop, // nr23
    (void (*)(uint8_t))&nop, // nr24
    (void (*)(uint8_t))&nop, // nr30
    (void (*)(uint8_t))&nop, // nr31
    (void (*)(uint8_t))&nop, // nr32
    (void (*)(uint8_t))&nop, // nr33
    (void (*)(uint8_t))&nop, // nr34
    (void (*)(uint8_t))&nop, // rsvd4
    (void (*)(uint8_t))&nop, // nr41
    (void (*)(uint8_t))&nop, // nr42
    (void (*)(uint8_t))&nop, // nr43
    (void (*)(uint8_t))&nop, // nr44
    (void (*)(uint8_t))&nop, // nr50
    (void (*)(uint8_t))&nop, // nr51
    (void (*)(uint8_t))&nop, // nr52
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // rsvd5
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    (void (*)(uint8_t))&nop, // wave_pat
    &lcdc, // lcdc
    &stat, // stat
    (void (*)(uint8_t))&nop, // scy
    (void (*)(uint8_t))&nop, // scx
    NULL, // ly
    NULL, // lyc
    &dma, // dma
    (void (*)(uint8_t))&nop, // bgp
    (void (*)(uint8_t))&nop, // obp0
    (void (*)(uint8_t))&nop, // obp1
    (void (*)(uint8_t))&nop, // wy
    (void (*)(uint8_t))&nop, // wx
    NULL, // rsvd6
    &key1, // key1
    (void (*)(uint8_t))&nop, // rsvd6
    &vbk, // vbk
    NULL, // rsvd6
    &hdma1, // hdma1
    &hdma2, // hdma2
    &hdma3, // hdma3
    &hdma4, // hdma4
    &hdma5, // hdma5
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    &bcps, // bcps
    &bcpd, // bcpd
    &ocps, // ocps
    &ocpd, // ocpd
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    &svbk, // svbk
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    NULL, // rsvd6
    &int_enable, // int_enable
};

static const char *reg_names[256] =
{
    "p1",
    "sb",
    "sc",
    "reserved 0x03",
    "div",
    "tima",
    "tma",
    "tac",
    "reserved 0x08",
    "reserved 0x09",
    "reserved 0x0A",
    "reserved 0x0B",
    "reserved 0x0C",
    "reserved 0x0D",
    "reserved 0x0E",
    "int_flag",
    "nr10",
    "nr11",
    "nr12",
    "nr13",
    "nr14",
    "reserved 0x15",
    "nr21",
    "nr22",
    "nr23",
    "nr24",
    "nr30",
    "nr31",
    "nr32",
    "nr33",
    "nr34",
    "reserved 0x1F",
    "nr41",
    "nr42",
    "nr43",
    "nr44",
    "nr50",
    "nr51",
    "nr52",
    "reserved 0x27",
    "reserved 0x28",
    "reserved 0x29",
    "reserved 0x2A",
    "reserved 0x2B",
    "reserved 0x2C",
    "reserved 0x2D",
    "reserved 0x2E",
    "reserved 0x2F",
    "wave_pat 0x30",
    "wave_pat 0x31",
    "wave_pat 0x32",
    "wave_pat 0x33",
    "wave_pat 0x34",
    "wave_pat 0x35",
    "wave_pat 0x36",
    "wave_pat 0x37",
    "wave_pat 0x38",
    "wave_pat 0x39",
    "wave_pat 0x3A",
    "wave_pat 0x3B",
    "wave_pat 0x3C",
    "wave_pat 0x3D",
    "wave_pat 0x3E",
    "wave_pat 0x3F",
    "lcdc",
    "stat",
    "scy",
    "scx",
    "ly",
    "lyc",
    "dma",
    "bgp",
    "obp0",
    "obp1",
    "wy",
    "wx",
    "reserved 0x4C",
    "key1",
    "reserved 0x4E",
    "vbk",
    "reserved 0x50",
    "hdma1",
    "hdma2",
    "hdma3",
    "hdma4",
    "hdma5",
    "reserved 0x56",
    "reserved 0x57",
    "reserved 0x58",
    "reserved 0x59",
    "reserved 0x5A",
    "reserved 0x5B",
    "reserved 0x5C",
    "reserved 0x5D",
    "reserved 0x5E",
    "reserved 0x5F",
    "reserved 0x60",
    "reserved 0x61",
    "reserved 0x62",
    "reserved 0x63",
    "reserved 0x64",
    "reserved 0x65",
    "reserved 0x66",
    "reserved 0x67",
    "bcps",
    "bcpd",
    "ocpa",
    "ocpd",
    "reserved 0x6C",
    "reserved 0x6D",
    "reserved 0x6E",
    "reserved 0x6F",
    "svbk",
    "reserved 0x71",
    "reserved 0x72",
    "reserved 0x73",
    "reserved 0x74",
    "reserved 0x75",
    "reserved 0x76",
    "reserved 0x77",
    "reserved 0x78",
    "reserved 0x79",
    "reserved 0x7A",
    "reserved 0x7B",
    "reserved 0x7C",
    "reserved 0x7D",
    "reserved 0x7E",
    "reserved 0x7F",
    "reserved 0x80",
    "reserved 0x81",
    "reserved 0x82",
    "reserved 0x83",
    "reserved 0x84",
    "reserved 0x85",
    "reserved 0x86",
    "reserved 0x87",
    "reserved 0x88",
    "reserved 0x89",
    "reserved 0x8A",
    "reserved 0x8B",
    "reserved 0x8C",
    "reserved 0x8D",
    "reserved 0x8E",
    "reserved 0x8F",
    "reserved 0x90",
    "reserved 0x91",
    "reserved 0x92",
    "reserved 0x93",
    "reserved 0x94",
    "reserved 0x95",
    "reserved 0x96",
    "reserved 0x97",
    "reserved 0x98",
    "reserved 0x99",
    "reserved 0x9A",
    "reserved 0x9B",
    "reserved 0x9C",
    "reserved 0x9D",
    "reserved 0x9E",
    "reserved 0x9F",
    "reserved 0xA0",
    "reserved 0xA1",
    "reserved 0xA2",
    "reserved 0xA3",
    "reserved 0xA4",
    "reserved 0xA5",
    "reserved 0xA6",
    "reserved 0xA7",
    "reserved 0xA8",
    "reserved 0xA9",
    "reserved 0xAA",
    "reserved 0xAB",
    "reserved 0xAC",
    "reserved 0xAD",
    "reserved 0xAE",
    "reserved 0xAF",
    "reserved 0xB0",
    "reserved 0xB1",
    "reserved 0xB2",
    "reserved 0xB3",
    "reserved 0xB4",
    "reserved 0xB5",
    "reserved 0xB6",
    "reserved 0xB7",
    "reserved 0xB8",
    "reserved 0xB9",
    "reserved 0xBA",
    "reserved 0xBB",
    "reserved 0xBC",
    "reserved 0xBD",
    "reserved 0xBE",
    "reserved 0xBF",
    "reserved 0xC0",
    "reserved 0xC1",
    "reserved 0xC2",
    "reserved 0xC3",
    "reserved 0xC4",
    "reserved 0xC5",
    "reserved 0xC6",
    "reserved 0xC7",
    "reserved 0xC8",
    "reserved 0xC9",
    "reserved 0xCA",
    "reserved 0xCB",
    "reserved 0xCC",
    "reserved 0xCD",
    "reserved 0xCE",
    "reserved 0xCF",
    "reserved 0xD0",
    "reserved 0xD1",
    "reserved 0xD2",
    "reserved 0xD3",
    "reserved 0xD4",
    "reserved 0xD5",
    "reserved 0xD6",
    "reserved 0xD7",
    "reserved 0xD8",
    "reserved 0xD9",
    "reserved 0xDA",
    "reserved 0xDB",
    "reserved 0xDC",
    "reserved 0xDD",
    "reserved 0xDE",
    "reserved 0xDF",
    "reserved 0xE0",
    "reserved 0xE1",
    "reserved 0xE2",
    "reserved 0xE3",
    "reserved 0xE4",
    "reserved 0xE5",
    "reserved 0xE6",
    "reserved 0xE7",
    "reserved 0xE8",
    "reserved 0xE9",
    "reserved 0xEA",
    "reserved 0xEB",
    "reserved 0xEC",
    "reserved 0xED",
    "reserved 0xEE",
    "reserved 0xEF",
    "reserved 0xF0",
    "reserved 0xF1",
    "reserved 0xF2",
    "reserved 0xF3",
    "reserved 0xF4",
    "reserved 0xF5",
    "reserved 0xF6",
    "reserved 0xF7",
    "reserved 0xF8",
    "reserved 0xF9",
    "reserved 0xFA",
    "reserved 0xFB",
    "reserved 0xFC",
    "reserved 0xFD",
    "reserved 0xFE",
    "int_enable"
};

void io_outb(uint8_t reg, uint8_t val)
{
    if (io_handlers[reg] == NULL)
    {
        fprintf(stderr, "No handler for write (0x%02X) to I/O register %s (0x%02X)!\n", (unsigned)val, reg_names[reg], (unsigned)reg);
        exit(1);
    }
    ((void (*)(uint8_t, int))io_handlers[reg])(val, reg);
}
