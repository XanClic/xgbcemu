#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "gbc.h"

static void (*handle[256])(void) = { NULL };

static void mem_writeb(uintptr_t addr, uint8_t value)
{
    uint8_t *caddr = (uint8_t *)&memory[addr];

    if ((addr >= 0xC000) && (addr < 0xDE00))
    {
        caddr[0x0000] = value;
        caddr[0x2000] = value;
    }
    else if ((addr >= 0xE000) && (addr < 0xFE00))
    {
        caddr[ 0x0000] = value;
        caddr[-0x2000] = value;
    }
    else if (((addr >= 0xFF00) && (addr < 0xFF4C)) || (addr == 0xFFFF))
    {
        printf("I/O write: 0x%02X -> 0x%04X\n", value, addr);
        exit(0);
    }
    else
        *caddr = value;
}

/*
static void mem_writew(uintptr_t addr, uint16_t value)
{
    uint16_t *caddr = (uint16_t *)&memory[addr];

    if ((addr >= 0xC000) && (addr < 0xDE00))
    {
        caddr[0x0000] = value;
        caddr[0x2000] = value;
    }
    else if ((addr >= 0xE000) && (addr < 0xFE00))
    {
        caddr[ 0x0000] = value;
        caddr[-0x2000] = value;
    }
    else if (((addr >= 0xFF00) && (addr < 0xFF4C)) || (addr == 0xFFFF))
    {
        printf("I/O write: 0x%04X -> 0x%04X\n", value, addr);
        exit(0);
    }
    else
        *caddr = value;
}
*/

static void nop(void)
{
    ip++;
}

static void jr(void)
{
    ip += ((signed char *)memory)[ip + 1];
}

static void jrz(void)
{
    if (f & FLAG_ZERO)
        jr();
    else
        ip += 2;
}

static void ld_a_s(void)
{
    ip++;
    a = memory[ip++];
}

static void xor_a_a(void)
{
    ip++;
    a = 0;
}

static void jp(void)
{
    ip++;
    ip = *((uint16_t *)&memory[ip]);
}

static void ld_ffn_a(void)
{
    ip++;
    mem_writeb(0xFF00 + memory[ip++], a);
}

static void ld_nn_a(void)
{
    ip++;
    mem_writeb(*((uint16_t *)&memory[ip]), a);
}

static void di_(void)
{
    ip++;
    ints_enabled = 0;
}

static void ei_(void)
{
    ip++;
    ints_enabled = 1;
}

static void cp_s(void)
{
    uint32_t eflags;

    ip++;
    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,bl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "b"(memory[ip]));
    if (eflags & (1 << 6))
        f |= FLAG_ZERO;
    if (eflags & (1 << 0))
        f |= FLAG_CRY;
    if (eflags & (1 << 4))
        f |= FLAG_HCRY;
    ip++;
}

void run(void)
{
    handle[0x00] = &nop;
    handle[0x18] = &jr;
    handle[0x28] = &jrz;
    handle[0x3E] = &ld_a_s;
    handle[0xAF] = &xor_a_a;
    handle[0xC3] = &jp;
    handle[0xE0] = &ld_ffn_a;
    handle[0xEA] = &ld_nn_a;
    handle[0xF3] = &di_;
    handle[0xFB] = &ei_;
    handle[0xFE] = &cp_s;

    ip = 0x0100;
    sp = 0xFFFE;
    af = 0x11B0;
    bc = 0x0013;
    de = 0x00D8;
    hl = 0x014D;

    io_regs->tima = 0x00;
    io_regs->tma = 0x00;
    io_regs->tac = 0x00;
    io_regs->nr10 = 0x80;
    io_regs->nr11 = 0xBF;
    io_regs->nr12 = 0xF3;
    io_regs->nr14 = 0xBF;
    io_regs->nr21 = 0x3F;
    io_regs->nr22 = 0x00;
    io_regs->nr24 = 0xBF;
    io_regs->nr30 = 0x7F;
    io_regs->nr31 = 0xFF;
    io_regs->nr32 = 0x9F;
    io_regs->nr33 = 0xBF;
    io_regs->nr41 = 0xFF;
    io_regs->nr42 = 0x00;
    io_regs->nr43 = 0x00;
    io_regs->nr44 = 0xBF;
    io_regs->nr50 = 0x77;
    io_regs->nr51 = 0xF3;
    io_regs->nr52 = 0xF1;
    io_regs->lcdc = 0x91;
    io_regs->scy = 0x00;
    io_regs->scx = 0x00;
    io_regs->lyc = 0x00;
    io_regs->bgp = 0xFC;
    io_regs->obp0 = 0xFF;
    io_regs->obp1 = 0xFF;
    io_regs->wy = 0x00;
    io_regs->wx = 0x00;
    io_regs->int_enable = 0x00;

    for (;;)
    {
        if (handle[(int)memory[ip]] == NULL)
        {
            printf("Unknown opcode 0x%02X\n", memory[ip]);
            break;
        }

        handle[(int)memory[ip]]();
    }
}
