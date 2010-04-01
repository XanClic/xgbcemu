#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

static void store_and_redraw(uint8_t value, int offset)
{
    memory[0xFF00 + offset] = value;
    redraw();
}

static void nop(uint8_t value, int offset)
{
    memory[0xFF00 + offset] = value;
}

static void lcdc(uint8_t value)
{
    io_regs->lcdc = value;
    if (!(value & (1 << 7)))
        lcd_on = 0;
    else
        lcd_on = 1;
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

static void (*const io_handlers[256])(uint8_t value) =
{
    &p1, // p1
    (void (*)(uint8_t))&nop, // sb
    (void (*)(uint8_t))&nop, // sc
    NULL, // rsvd1
    NULL, // div
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
    NULL, // nr10
    NULL, // nr11
    NULL, // nr12
    NULL, // nr13
    NULL, // nr14
    NULL, // rsvd3
    NULL, // nr21
    NULL, // nr22
    NULL, // nr23
    NULL, // nr24
    NULL, // nr30
    NULL, // nr31
    NULL, // nr32
    NULL, // nr33
    NULL, // nr34
    NULL, // rsvd4
    NULL, // nr41
    NULL, // nr42
    NULL, // nr43
    NULL, // nr44
    NULL, // nr50
    NULL, // nr51
    NULL, // nr52
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // rsvd5
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    NULL, // wave_pat
    &lcdc, // lcdc
    (void (*)(uint8_t))&nop, // stat
    (void (*)(uint8_t))&store_and_redraw, // scy
    (void (*)(uint8_t))&store_and_redraw, // scx
    NULL, // ly
    NULL, // lyc
    NULL, // dma
    (void (*)(uint8_t))&store_and_redraw, // bgp
    (void (*)(uint8_t))&store_and_redraw, // obp0
    (void (*)(uint8_t))&store_and_redraw, // obp1
    (void (*)(uint8_t))&store_and_redraw, // wy
    (void (*)(uint8_t))&store_and_redraw, // wx
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
    "reserved 0x4D",
    "reserved 0x4E",
    "reserved 0x4F",
    "reserved 0x50",
    "reserved 0x51",
    "reserved 0x52",
    "reserved 0x53",
    "reserved 0x54",
    "reserved 0x55",
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
    "reserved 0x68",
    "reserved 0x69",
    "reserved 0x6A",
    "reserved 0x6B",
    "reserved 0x6C",
    "reserved 0x6D",
    "reserved 0x6E",
    "reserved 0x6F",
    "reserved 0x70",
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
        fprintf(stderr, "No handler for write to I/O register %s!\n", reg_names[reg]);
        exit(1);
    }
    ((void (*)(uint8_t, int))io_handlers[reg])(val, reg);
}
