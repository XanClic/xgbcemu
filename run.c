#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gbc.h"

// #define DUMP

// #define DUMP_REGS

// #define HALT_ON_RST

#define TRUE_TIMING

#define X86_ZF (1 << 6)
#define X86_CF (1 << 0)
#define X86_AF (1 << 4)

#define nn mem_readw(ip)

#ifdef TRUE_TIMING
static uint64_t new_tsc, last_tsc;
static uint32_t diff;
#endif

extern void (*const handle0xCB[64])(void);
extern const int cycles[256];
extern const int cycles0xCB[256];

static void call(void);
static void jp(void);
static void jr(void);
static void ret(void);

static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (uint64_t)lo | ((uint64_t)hi << 32);
}

static void nop(void)
{
    #ifdef DUMP
    printf("NOP\n");
    #endif
}

static void ld_bc_nn(void)
{
    #ifdef DUMP
    printf("LD BC, 0x%04X: BC == 0x%04X\n", (unsigned)nn, (unsigned)bc);
    #endif
    bc = nn;
    ip += 2;
}

static void ld__bc_a(void)
{
    #ifdef DUMP
    printf("LD (BC), A: BC == 0x%04X; A == 0x%02X\n", (unsigned)bc, (unsigned)a);
    #endif
    mem_writeb(bc, a);
}

static void inc_bc(void)
{
    #ifdef DUMP
    printf("INC BC: BC == 0x%04X\n", (unsigned)bc);
    #endif
    bc++;
}

static void inc_b(void)
{
    #ifdef DUMP
    printf("INC B: B == 0x%02X\n", (unsigned)b);
    #endif

    f &= FLAG_CRY;
    if (!++b)
        f |= FLAG_ZERO;
    if (!(b & 0xF))
        f |= FLAG_HCRY;
}

static void dec_b(void)
{
    #ifdef DUMP
    printf("DEC B: B == 0x%02X\n", (unsigned)b);
    #endif

    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(b & 0xF))
        f |= FLAG_HCRY;
    if (!--b)
        f |= FLAG_ZERO;
}

static void ld_b_n(void)
{
    #ifdef DUMP
    printf("LD B, 0x%02X: B == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)b);
    #endif
    b = mem_readb(ip++);
}

static void rlca(void)
{
    #ifdef DUMP
    printf("RLCA: A == 0x%02X\n", (unsigned)a);
    #endif
    f = (a & 0x80) ? FLAG_CRY : 0;
    __asm__ __volatile__ ("rol al,1" : "=a"(a) : "a"(a));
    if (!a)
        f |= FLAG_ZERO;
}

static void ld__nn_sp(void)
{
    #ifdef DUMP
    printf("LD (0x%04X), SP: SP == 0x%04X\n", (unsigned)nn, (unsigned)sp);
    #endif
    mem_writew(nn, sp);
    ip += 2;
}

static void add_hl_bc(void)
{
    int result = hl + bc;

    #ifdef DUMP
    printf("ADD HL, BC: HL == 0x%04X; BC == 0x%04X\n", (unsigned)hl, (unsigned)bc);
    #endif

    f &= FLAG_ZERO;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if ((hl & 0xFFF) + (bc & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    hl = result;
}

static void ld_a__bc(void)
{
    #ifdef DUMP
    printf("LD A, (BC): A == 0x%02X; BC == 0x%04X\n", (unsigned)a, (unsigned)bc);
    #endif
    a = mem_readb(bc);
}

static void dec_bc(void)
{
    #ifdef DUMP
    printf("DEC BC: BC == 0x%04X\n", (unsigned)bc);
    #endif
    bc--;
}

static void inc_c(void)
{
    #ifdef DUMP
    printf("INC C: C == 0x%02X\n", (unsigned)c);
    #endif

    f &= FLAG_CRY;
    if (!++c)
        f |= FLAG_ZERO;
    if (!(c & 0xF))
        f |= FLAG_HCRY;
}

static void dec_c(void)
{
    #ifdef DUMP
    printf("DEC C: C == 0x%02X\n", (unsigned)c);
    #endif

    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(c & 0xF))
        f |= FLAG_HCRY;
    if (!--c)
        f |= FLAG_ZERO;
}

static void ld_c_n(void)
{
    #ifdef DUMP
    printf("LD C, 0x%02X: C == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)c);
    #endif
    c = mem_readb(ip++);
}

static void rrca(void)
{
    #ifdef DUMP
    printf("RRCA: A == 0x%02X\n", (unsigned)a);
    #endif
    f = (a & 0x01) ? FLAG_CRY : 0;
    __asm__ __volatile__ ("ror al,1" : "=a"(a) : "a"(a));
    if (!a)
        f |= FLAG_ZERO;
}

static void prefix0x10(void)
{
    switch (mem_readb(ip))
    {
        case 0x00:
            printf("STOP\n");
            if (!(io_regs->key1 & 1))
                exit(0);
            else
            {
                double_speed ^= 1;
                printf("Using %s speed\n", double_speed ? "double" : "single");
            }
        default:
            printf("Unknown opcode 0x%02X, prefixed by 0x10.\n", (unsigned)mem_readb(ip));
            exit(1);
    }
}

static void ld_de_nn(void)
{
    #ifdef DUMP
    printf("LD DE, 0x%04X: DE == 0x%04X\n", (unsigned)nn, (unsigned)de);
    #endif
    de = nn;
    ip += 2;
}

static void ld__de_a(void)
{
    #ifdef DUMP
    printf("LD (DE), A: DE == 0x%04X; A == 0x%02X\n", (unsigned)de, (unsigned)a);
    #endif
    mem_writeb(de, a);
}

static void inc_de(void)
{
    #ifdef DUMP
    printf("INC DE: DE == 0x%04X\n", (unsigned)de);
    #endif
    de++;
}

static void inc_d(void)
{
    #ifdef DUMP
    printf("INC D: D == 0x%02X\n", (unsigned)d);
    #endif

    f &= FLAG_CRY;
    if (!++d)
        f |= FLAG_ZERO;
    if (!(d & 0xF))
        f |= FLAG_HCRY;
}

static void dec_d(void)
{
    #ifdef DUMP
    printf("DEC D: D == 0x%02X\n", (unsigned)d);
    #endif

    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(d & 0xF))
        f |= FLAG_HCRY;
    if (!--d)
        f |= FLAG_ZERO;
}

static void ld_d_n(void)
{
    #ifdef DUMP
    printf("LD D, 0x%02X: D == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)d);
    #endif
    d = mem_readb(ip++);
}

static void rla(void)
{
    int cry = f & FLAG_CRY;

    #ifdef DUMP
    printf("RLA: A == 0x%02X\n", (unsigned)a);
    #endif

    f = (a & 0x80) ? FLAG_CRY : 0;
    a = ((a << 1) & 0xFF) | !!cry;
    if (!a)
        f |= FLAG_ZERO;
}

static void jr(void)
{
    #ifdef DUMP
    int off = (int)(signed char)mem_readb(ip);
    printf("JR %i: 0x%04X → 0x%04X\n", (int)off, (unsigned)(ip - 1), (unsigned)(ip + off + 1));
    #endif
    ip += (int)(signed char)mem_readb(ip) + 1;
}

static void add_hl_de(void)
{
    int result = hl + de;

    #ifdef DUMP
    printf("ADD HL, DE: HL == 0x%04X; DE == 0x%04X\n", (unsigned)hl, (unsigned)de);
    #endif

    f &= FLAG_ZERO;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if ((hl & 0xFFF) + (bc & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    hl = result;
}

static void ld_a__de(void)
{
    #ifdef DUMP
    printf("LD A, (DE): A == 0x%02X; DE == 0x%04X\n", (unsigned)a, (unsigned)de);
    #endif
    a = mem_readb(de);
}

static void dec_de(void)
{
    #ifdef DUMP
    printf("DEC DE: DE == 0x%04X\n", (unsigned)de);
    #endif
    de--;
}

static void inc_e(void)
{
    #ifdef DUMP
    printf("INC E: E == 0x%02X\n", (unsigned)e);
    #endif

    f &= FLAG_CRY;
    if (!++e)
        f |= FLAG_ZERO;
    if (!(e & 0xF))
        f |= FLAG_HCRY;
}

static void dec_e(void)
{
    #ifdef DUMP
    printf("DEC E: E == 0x%02X\n", (unsigned)e);
    #endif

    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(e & 0xF))
        f |= FLAG_HCRY;
    if (!--e)
        f |= FLAG_ZERO;
}

static void ld_e_n(void)
{
    #ifdef DUMP
    printf("LD E, 0x%02X: E == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)e);
    #endif
    e = mem_readb(ip++);
}

static void rra(void)
{
    int cry = f & FLAG_CRY;

    #ifdef DUMP
    printf("RRA: A == 0x%02X\n", (unsigned)a);
    #endif

    f = (a & 0x01) ? FLAG_CRY : 0;
    a = (a >> 1) | (!!cry << 7);
    if (!a)
        f |= FLAG_ZERO;
}

static void jrnz(void)
{
    if (!(f & FLAG_ZERO))
    {
        #ifdef DUMP
        printf("JRNZ, branching\n");
        #endif
        jr();
    }
    else
    {
        #ifdef DUMP
        printf("JRNZ, staying\n");
        #endif
        ip++;
    }
}

static void ld_hl_nn(void)
{
    #ifdef DUMP
    printf("LD HL, 0x%04X: HL == 0x%04X\n", (unsigned)nn, (unsigned)hl);
    #endif
    hl = nn;
    ip += 2;
}

static void ldi__hl_a(void)
{
    #ifdef DUMP
    printf("LDI (HL), A: HL == 0x%04X; A == 0x%02X\n", (unsigned)hl, (unsigned)a);
    #endif
    mem_writeb(hl++, a);
}

static void inc_hl(void)
{
    #ifdef DUMP
    printf("INC HL: HL == 0x%04X\n", (unsigned)hl);
    #endif
    hl++;
}

static void inc_h(void)
{
    #ifdef DUMP
    printf("INC H: H == 0x%02X\n", (unsigned)h);
    #endif

    f &= FLAG_CRY;
    if (!++h)
        f |= FLAG_ZERO;
    if (!(h & 0xF))
        f |= FLAG_HCRY;
}

static void dec_h(void)
{
    #ifdef DUMP
    printf("DEC H: H == 0x%02X\n", (unsigned)h);
    #endif

    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(h & 0xF))
        f |= FLAG_HCRY;
    if (!--h)
        f |= FLAG_ZERO;
}

static void ld_h_n(void)
{
    #ifdef DUMP
    printf("LD H, 0x%02X: H == 0x%02X\n", mem_readb(ip), h);
    #endif
    h = mem_readb(ip++);
}

static void daa(void)
{
    int old_f = f;
    uint16_t new_a = a;

    #ifdef DUMP
    printf("DAA: A == 0x%02X; F == 0x%02X\n", (unsigned)a, (unsigned)f);
    #endif

    f &= FLAG_SUB;

    if (old_f & FLAG_SUB)
    {
        if (((a & 0x0F) > 0x09) || (old_f & FLAG_HCRY))
            new_a -= 0x06;
        if ((a > 0x99) || (old_f & FLAG_CRY))
        {
            new_a -= 0x60;
            f |= FLAG_CRY;
        }
    }
    else
    {
        if (((a & 0x0F) > 0x09) || (old_f & FLAG_HCRY))
            new_a += 0x06;
        if ((a > 0x99) || (old_f & FLAG_CRY))
        {
            new_a += 0x60;
            f |= FLAG_CRY;
        }
        else
            f &= ~FLAG_CRY;
    }

    a = new_a & 0xFF;
    if (!a)
        f |= FLAG_ZERO;
    if ((new_a & 0xFF00) || (old_f & FLAG_CRY))
        f |= FLAG_CRY;
}

static void jrz(void)
{
    if (f & FLAG_ZERO)
    {
        #ifdef DUMP
        printf("JRZ, branching\n");
        #endif
        jr();
    }
    else
    {
        #ifdef DUMP
        printf("JRZ, staying\n");
        #endif
        ip++;
    }
}

static void add_hl_hl(void)
{
    #ifdef DUMP
    printf("ADD HL, HL: HL == 0x%02X\n", (unsigned)hl);
    #endif

    f &= FLAG_ZERO;
    if (hl & 0x8000)
        f |= FLAG_CRY;
    if (hl & 0x0800)
        f |= FLAG_HCRY;

    hl <<= 1;
}

static void ldi_a__hl(void)
{
    #ifdef DUMP
    printf("LDI A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    a = mem_readb(hl++);
}

static void dec_hl(void)
{
    #ifdef DUMP
    printf("DEC HL: HL == 0x%04X\n", (unsigned)hl);
    #endif
    hl--;
}

static void inc_l(void)
{
    #ifdef DUMP
    printf("INC L: L == 0x%02X\n", (unsigned)l);
    #endif

    f &= FLAG_CRY;
    if (!++l)
        f |= FLAG_ZERO;
    if (!(l & 0xF))
        f |= FLAG_HCRY;
}

static void dec_l(void)
{
    #ifdef DUMP
    printf("DEC L: L == 0x%02X\n", (unsigned)l);
    #endif

    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(l & 0xF))
        f |= FLAG_HCRY;
    if (!--l)
        f |= FLAG_ZERO;
}

static void ld_l_n(void)
{
    #ifdef DUMP
    printf("LD L, 0x%02X: L == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)l);
    #endif
    l = mem_readb(ip++);
}

static void cpl_a(void)
{
    #ifdef DUMP
    printf("CPL A: A == 0x%02X\n", (unsigned)a);
    #endif
    a = ~a & 0xFF;
    f = FLAG_SUB | FLAG_HCRY;
}


static void jrnc(void)
{
    if (!(f & FLAG_CRY))
    {
        #ifdef DUMP
        printf("JRNC, branching\n");
        #endif
        jr();
    }
    else
    {
        #ifdef DUMP
        printf("JRNC, staying\n");
        #endif
        ip++;
    }
}

static void ld_sp_nn(void)
{
    #ifdef DUMP
    printf("LD SP, 0x%04X: SP == 0x%04X\n", (unsigned)nn, (unsigned)sp);
    #endif
    sp = nn;
    ip += 2;
}

static void ldd__hl_a(void)
{
    #ifdef DUMP
    printf("LDD (HL), A: HL == 0x%04X; A == 0x%02X\n", (unsigned)hl, (unsigned)a);
    #endif
    mem_writeb(hl--, a);
}

static void inc_sp(void)
{
    #ifdef DUMP
    printf("INC SP: SP == 0x%04X\n", (unsigned)sp);
    #endif
    sp++;
}

static void inc__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("INC (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f &= FLAG_CRY;
    if (!++val)
        f |= FLAG_ZERO;
    if (!(val & 0xF))
        f |= FLAG_HCRY;
    mem_writeb(hl, val);
}

static void dec__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("DEC (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(val & 0xF))
        f |= FLAG_HCRY;
    if (!--val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void ld__hl_n(void)
{
    #ifdef DUMP
    printf("LD (HL), 0x%02X: HL == 0x%04X\n", (unsigned)mem_readb(ip), (unsigned)hl);
    #endif
    mem_writeb(hl, mem_readb(ip++));
}

static void scf(void)
{
    #ifdef DUMP
    printf("SCF: F == 0x%02X\n", (unsigned)f);
    #endif
    f |= FLAG_CRY;
}

static void jrc(void)
{
    if (f & FLAG_CRY)
    {
        #ifdef DUMP
        printf("JRC, branching\n");
        #endif
        jr();
    }
    else
    {
        #ifdef DUMP
        printf("JRC, staying\n");
        #endif
        ip++;
    }
}

static void add_hl_sp(void)
{
    int result = hl + sp;

    #ifdef DUMP
    printf("ADD HL, SP: HL == 0x%04X; SP == 0x%04X\n", (unsigned)hl, (unsigned)sp);
    #endif

    f &= FLAG_ZERO;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if ((hl & 0xFFF) + (bc & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    hl = result;
}

static void ldd_a__hl(void)
{
    #ifdef DUMP
    printf("LDD A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    a = mem_readb(hl--);
}

static void dec_sp(void)
{
    #ifdef DUMP
    printf("DEC SP: SP == 0x%04X\n", (unsigned)sp);
    #endif
    sp--;
}

static void inc_a(void)
{
    #ifdef DUMP
    printf("INC A: A == 0x%02X\n", (unsigned)a);
    #endif

    f &= FLAG_CRY;
    if (!++a)
        f |= FLAG_ZERO;
    if (!(a & 0xF))
        f |= FLAG_HCRY;
}

static void dec_a(void)
{
    #ifdef DUMP
    printf("DEC A: A == 0x%02X\n", (unsigned)a);
    #endif

    f = (f & FLAG_CRY) | FLAG_SUB;
    if (!(a & 0xF))
        f |= FLAG_HCRY;
    if (!--a)
        f |= FLAG_ZERO;
}

static void ld_a_s(void)
{
    #ifdef DUMP
    printf("LD A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif
    a = mem_readb(ip++);
}

static void ccf(void)
{
    #ifdef DUMP
    printf("CCF: F == 0x%02X\n", (unsigned)f);
    #endif
    f ^= FLAG_CRY;
}

static void ld_b_b(void)
{
    #ifdef DUMP
    printf("LD B, B: B == 0x%02X\n", (unsigned)b);
    #endif
}

static void ld_b_c(void)
{
    #ifdef DUMP
    printf("LD B, C: B == 0x%02X; C == 0x%02X\n", (unsigned)b, (unsigned)c);
    #endif
    b = c;
}

static void ld_b_d(void)
{
    #ifdef DUMP
    printf("LD B, D: B == 0x%02X; D == 0x%02X\n", (unsigned)b, (unsigned)d);
    #endif
    b = d;
}

static void ld_b_e(void)
{
    #ifdef DUMP
    printf("LD B, E: B == 0x%02X; E == 0x%02X\n", (unsigned)b, (unsigned)e);
    #endif
    b = e;
}

static void ld_b_h(void)
{
    #ifdef DUMP
    printf("LD B, H: B == 0x%02X; H == 0x%02X\n", (unsigned)b, (unsigned)h);
    #endif
    b = h;
}

static void ld_b_l(void)
{
    #ifdef DUMP
    printf("LD B, L: B == 0x%02X; L == 0x%02X\n", (unsigned)b, (unsigned)l);
    #endif
    b = l;
}

static void ld_b__hl(void)
{
    #ifdef DUMP
    printf("LD B, (HL): B == 0x%02X; HL == 0x%04X\n", (unsigned)b, (unsigned)hl);
    #endif
    b = mem_readb(hl);
}

static void ld_b_a(void)
{
    #ifdef DUMP
    printf("LD B, A: B == 0x%02X; A == 0x%02X\n", (unsigned)b, (unsigned)a);
    #endif
    b = a;
}

static void ld_c_b(void)
{
    #ifdef DUMP
    printf("LD C, B: C == 0x%02X; B == 0x%02X\n", (unsigned)c, (unsigned)b);
    #endif
    c = b;
}

static void ld_c_c(void)
{
    #ifdef DUMP
    printf("LD C, C: C == 0x%02X\n", (unsigned)c);
    #endif
}

static void ld_c_d(void)
{
    #ifdef DUMP
    printf("LD C, D: C == 0x%02X; D == 0x%02X\n", (unsigned)c, (unsigned)d);
    #endif
    c = d;
}

static void ld_c_e(void)
{
    #ifdef DUMP
    printf("LD C, E: C == 0x%02X; E == 0x%02X\n", (unsigned)c, (unsigned)e);
    #endif
    c = e;
}

static void ld_c_h(void)
{
    #ifdef DUMP
    printf("LD C, H: C == 0x%02X; H == 0x%02X\n", (unsigned)c, (unsigned)h);
    #endif
    c = h;
}

static void ld_c_l(void)
{
    #ifdef DUMP
    printf("LD C, L: C == 0x%02X; L == 0x%02X\n", (unsigned)c, (unsigned)l);
    #endif
    c = l;
}

static void ld_c__hl(void)
{
    #ifdef DUMP
    printf("LD C, (HL): C == 0x%02X; HL == 0x%04X\n", (unsigned)c, (unsigned)hl);
    #endif
    c = mem_readb(hl);
}

static void ld_c_a(void)
{
    #ifdef DUMP
    printf("LD C, A: C == 0x%02X; A == 0x%02X\n", (unsigned)c, (unsigned)a);
    #endif
    c = a;
}

static void ld_d_b(void)
{
    #ifdef DUMP
    printf("LD D, B: D == 0x%02X; B == 0x%02X\n", (unsigned)d, (unsigned)b);
    #endif
    d = b;
}

static void ld_d_c(void)
{
    #ifdef DUMP
    printf("LD D, C: D == 0x%02X; C == 0x%02X\n", (unsigned)d, (unsigned)c);
    #endif
    d = c;
}

static void ld_d_d(void)
{
    #ifdef DUMP
    printf("LD D, D: D == 0x%02X\n", (unsigned)d);
    #endif
}

static void ld_d_e(void)
{
    #ifdef DUMP
    printf("LD D, E: D == 0x%02X; E == 0x%02X\n", (unsigned)d, (unsigned)e);
    #endif
    d = e;
}

static void ld_d_h(void)
{
    #ifdef DUMP
    printf("LD D, H: D == 0x%02X; H == 0x%02X\n", (unsigned)d, (unsigned)h);
    #endif
    d = h;
}

static void ld_d_l(void)
{
    #ifdef DUMP
    printf("LD D, L: D == 0x%02X; L == 0x%02X\n", (unsigned)d, (unsigned)l);
    #endif
    d = l;
}

static void ld_d__hl(void)
{
    #ifdef DUMP
    printf("LD D, (HL): D == 0x%02X; HL == 0x%04X\n", (unsigned)d, (unsigned)hl);
    #endif
    d = mem_readb(hl);
}

static void ld_d_a(void)
{
    #ifdef DUMP
    printf("LD D, A: D == 0x%02X; A == 0x%02X\n", (unsigned)d, (unsigned)a);
    #endif
    d = a;
}

static void ld_e_b(void)
{
    #ifdef DUMP
    printf("LD E, B: E == 0x%02X; B == 0x%02X\n", (unsigned)e, (unsigned)b);
    #endif
    e = b;
}

static void ld_e_c(void)
{
    #ifdef DUMP
    printf("LD E, C: E == 0x%02X; C == 0x%02X\n", (unsigned)e, (unsigned)c);
    #endif
    e = c;
}

static void ld_e_d(void)
{
    #ifdef DUMP
    printf("LD E, D: E == 0x%02X; D == 0x%02X\n", (unsigned)e, (unsigned)d);
    #endif
    e = d;
}

static void ld_e_e(void)
{
    #ifdef DUMP
    printf("LD E, E: E == 0x%02X\n", (unsigned)e);
    #endif
}

static void ld_e_h(void)
{
    #ifdef DUMP
    printf("LD E, H: E == 0x%02X; H == 0x%02X\n", (unsigned)e, (unsigned)h);
    #endif
    e = h;
}

static void ld_e_l(void)
{
    #ifdef DUMP
    printf("LD E, L: E == 0x%02X; L == 0x%02X\n", (unsigned)e, (unsigned)l);
    #endif
    e = l;
}

static void ld_e__hl(void)
{
    #ifdef DUMP
    printf("LD E, (HL): E == 0x%02X; HL == 0x%04X\n", (unsigned)e, (unsigned)hl);
    #endif
    e = mem_readb(hl);
}

static void ld_e_a(void)
{
    #ifdef DUMP
    printf("LD E, A: E == 0x%02X; A == 0x%02X\n", (unsigned)e, (unsigned)a);
    #endif
    e = a;
}

static void ld_h_b(void)
{
    #ifdef DUMP
    printf("LD H, B: H == 0x%02X; B == 0x%02X\n", (unsigned)h, (unsigned)b);
    #endif
    h = b;
}

static void ld_h_c(void)
{
    #ifdef DUMP
    printf("LD H, C: D == 0x%02X; C == 0x%02X\n", (unsigned)h, (unsigned)c);
    #endif
    h = c;
}

static void ld_h_d(void)
{
    #ifdef DUMP
    printf("LD H, D: H == 0x%02X; D == 0x%02X\n", (unsigned)h, (unsigned)d);
    #endif
    h = d;
}

static void ld_h_e(void)
{
    #ifdef DUMP
    printf("LD H, E: H == 0x%02X; E == 0x%02X\n", (unsigned)h, (unsigned)e);
    #endif
    h = e;
}

static void ld_h_h(void)
{
    #ifdef DUMP
    printf("LD H, H: H == 0x%02X\n", (unsigned)h);
    #endif
}

static void ld_h_l(void)
{
    #ifdef DUMP
    printf("LD H, L: H == 0x%02X; L == 0x%02X\n", (unsigned)h, (unsigned)l);
    #endif
    h = l;
}

static void ld_h__hl(void)
{
    #ifdef DUMP
    printf("LD H, (HL): H == 0x%02X; HL == 0x%04X\n", (unsigned)h, (unsigned)hl);
    #endif
    h = mem_readb(hl);
}

static void ld_h_a(void)
{
    #ifdef DUMP
    printf("LD H, A: H == 0x%02X; A == 0x%02X\n", (unsigned)h, (unsigned)a);
    #endif
    h = a;
}

static void ld_l_b(void)
{
    #ifdef DUMP
    printf("LD L, B: L == 0x%02X; B == 0x%02X\n", (unsigned)l, (unsigned)b);
    #endif
    l = b;
}

static void ld_l_c(void)
{
    #ifdef DUMP
    printf("LD L, C: L == 0x%02X; C == 0x%02X\n", (unsigned)l, (unsigned)c);
    #endif
    l = c;
}

static void ld_l_d(void)
{
    #ifdef DUMP
    printf("LD L, D: L == 0x%02X; D == 0x%02X\n", (unsigned)l, (unsigned)d);
    #endif
    l = d;
}

static void ld_l_e(void)
{
    #ifdef DUMP
    printf("LD L, E: L == 0x%02X, E == 0x%02X\n", (unsigned)l, (unsigned)e);
    #endif
    l = e;
}

static void ld_l_h(void)
{
    #ifdef DUMP
    printf("LD L, H: L == 0x%02X; H == 0x%02X\n", (unsigned)l, (unsigned)h);
    #endif
    l = h;
}

static void ld_l_l(void)
{
    #ifdef DUMP
    printf("LD L, L: L == 0x%02X\n", (unsigned)l);
    #endif
}

static void ld_l__hl(void)
{
    #ifdef DUMP
    printf("LD L, (HL): L == 0x%02X; HL == 0x%04X\n", (unsigned)l, (unsigned)hl);
    #endif
    l = mem_readb(hl);
}

static void ld_l_a(void)
{
    #ifdef DUMP
    printf("LD L, A: L == 0x%02X; A == 0x%02X\n", (unsigned)l, (unsigned)a);
    #endif
    l = a;
}

static void ld__hl_b(void)
{
    #ifdef DUMP
    printf("LD (HL), B: HL == 0x%04X; B == 0x%02X\n", (unsigned)hl, (unsigned)b);
    #endif
    mem_writeb(hl, b);
}

static void ld__hl_c(void)
{
    #ifdef DUMP
    printf("LD (HL), C: HL == 0x%04X; C == 0x%02X\n", (unsigned)hl, (unsigned)c);
    #endif
    mem_writeb(hl, c);
}

static void ld__hl_d(void)
{
    #ifdef DUMP
    printf("LD (HL), D: HL == 0x%04X; D == 0x%02X\n", (unsigned)hl, (unsigned)d);
    #endif
    mem_writeb(hl, d);
}

static void ld__hl_e(void)
{
    #ifdef DUMP
    printf("LD (HL), E: HL == 0x%04X; E == 0x%02X\n", (unsigned)hl, (unsigned)e);
    #endif
    mem_writeb(hl, e);
}

static void ld__hl_h(void)
{
    #ifdef DUMP
    printf("LD (HL), H: HL == 0x%04X; H == 0x%02X\n", (unsigned)hl, (unsigned)h);
    #endif
    mem_writeb(hl, h);
}

static void ld__hl_l(void)
{
    #ifdef DUMP
    printf("LD (HL), L: HL == 0x%04X; L == 0x%02X\n", (unsigned)hl, (unsigned)l);
    #endif
    mem_writeb(hl, l);
}

static void halt(void)
{
    #ifdef DUMP
    printf("HALT\n");
    #endif
    interrupt_issued = 0;
    while (!interrupt_issued)
    {
        update_timer(1);
        generate_interrupts();
    }
}

static void ld__hl_a(void)
{
    #ifdef DUMP
    printf("LD (HL), A: HL == 0x%04X; A == 0x%02X\n", (unsigned)hl, (unsigned)a);
    #endif
    mem_writeb(hl, a);
}

static void ld_a_b(void)
{
    #ifdef DUMP
    printf("LD A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif
    a = b;
}

static void ld_a_c(void)
{
    #ifdef DUMP
    printf("LD A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif
    a = c;
}

static void ld_a_d(void)
{
    #ifdef DUMP
    printf("LD A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif
    a = d;
}

static void ld_a_e(void)
{
    #ifdef DUMP
    printf("LD A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif
    a = e;
}

static void ld_a_h(void)
{
    #ifdef DUMP
    printf("LD A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif
    a = h;
}

static void ld_a_l(void)
{
    #ifdef DUMP
    printf("LD A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif
    a = l;
}

static void ld_a__hl(void)
{
    #ifdef DUMP
    printf("LD A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    a = mem_readb(hl);
}

static void ld_a_a(void)
{
    #ifdef DUMP
    printf("LD A, A: A == 0x%02X\n", (unsigned)a);
    #endif
}

static void add_a_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(b));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void add_a_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(c));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void add_a_d(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(d));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void add_a_e(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(e));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void add_a_h(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(h));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void add_a_l(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(l));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void add_a__hl(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(hl)));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void add_a_a(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, A: A == 0x%02X\n", (unsigned)a);
    #endif
    __asm__ __volatile__ ("add al,al; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(b));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(b));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(c));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(c));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a_d(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(d));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(d));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a_e(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(e));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(e));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a_h(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(h));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(h));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a_l(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(l));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(l));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a__hl(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(hl)));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(hl)));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void adc_a_a(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, A: A == 0x%02X\n", (unsigned)a);
    #endif
    if (f & X86_CF)
        __asm__ __volatile__ ("stc; adc al,al; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a));
    else
        __asm__ __volatile__ ("clc; adc al,al; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void sub_a_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(b));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sub_a_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(c));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sub_a_d(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(d));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sub_a_e(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(e));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sub_a_h(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(h));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sub_a_l(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(l));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sub_a__hl(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(hl)));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sub_a_a(void)
{
    #ifdef DUMP
    printf("SUB A, A: A == 0x%02X\n", (unsigned)a);
    #endif

    a = 0;
    f = FLAG_SUB | FLAG_ZERO;
}

static void sbc_a_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(b));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(b));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sbc_a_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(c));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(c));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sbc_a_d(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(d));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(d));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sbc_a_e(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(e));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(e));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sbc_a_h(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(h));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(h));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sbc_a_l(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(l));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(l));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sbc_a__hl(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(hl)));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(hl)));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void sbc_a_a(void)
{
    #ifdef DUMP
    printf("SUB A, A: A == 0x%02X\n", (unsigned)a);
    #endif

    if (f & FLAG_CRY)
    {
        a = 0xFF;
        f = FLAG_SUB | FLAG_ZERO | FLAG_CRY | FLAG_HCRY;
    }
    else
    {
        a = 0;
        f = FLAG_SUB | FLAG_ZERO;
    }
}

static void and_a_b(void)
{
    #ifdef DUMP
    printf("AND A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif

    a &= b;
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void and_a_c(void)
{
    #ifdef DUMP
    printf("AND A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif

    a &= c;
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void and_a_d(void)
{
    #ifdef DUMP
    printf("AND A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif

    a &= d;
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void and_a_e(void)
{
    #ifdef DUMP
    printf("AND A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif

    a &= e;
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void and_a_h(void)
{
    #ifdef DUMP
    printf("AND A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif

    a &= h;
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void and_a_l(void)
{
    #ifdef DUMP
    printf("AND A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif

    a &= l;
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void and_a__hl(void)
{
    #ifdef DUMP
    printf("AND A, (HL): A == 0x%02X; HL == 0x%42X\n", (unsigned)a, (unsigned)hl);
    #endif

    a &= mem_readb(hl);
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void and_a_a(void)
{
    #ifdef DUMP
    printf("AND A, A: A == 0x%02X\n", (unsigned)a);
    #endif

    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void xor_a_b(void)
{
    #ifdef DUMP
    printf("XOR A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif

    a ^= b;
    f = a ? 0 : FLAG_ZERO;
}

static void xor_a_c(void)
{
    #ifdef DUMP
    printf("XOR A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif

    a ^= c;
    f = a ? 0 : FLAG_ZERO;
}

static void xor_a_d(void)
{
    #ifdef DUMP
    printf("XOR A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif

    a ^= d;
    f = a ? 0 : FLAG_ZERO;
}

static void xor_a_e(void)
{
    #ifdef DUMP
    printf("XOR A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif

    a ^= e;
    f = a ? 0 : FLAG_ZERO;
}

static void xor_a_h(void)
{
    #ifdef DUMP
    printf("XOR A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif

    a ^= h;
    f = a ? 0 : FLAG_ZERO;
}

static void xor_a_l(void)
{
    #ifdef DUMP
    printf("XOR A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif

    a ^= l;
    f = a ? 0 : FLAG_ZERO;
}

static void xor_a__hl(void)
{
    #ifdef DUMP
    printf("XOR A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif

    a ^= mem_readb(hl);
    f = a ? 0 : FLAG_ZERO;
}

static void xor_a_a(void)
{
    #ifdef DUMP
    printf("XOR A, A: A == 0x%02X\n", (unsigned)a);
    #endif

    a = 0;
    f = FLAG_ZERO;
}

static void or_a_b(void)
{
    #ifdef DUMP
    printf("OR A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif

    a |= b;
    f = a ? 0 : FLAG_ZERO;
}

static void or_a_c(void)
{
    #ifdef DUMP
    printf("OR A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif

    a |= c;
    f = a ? 0 : FLAG_ZERO;
}

static void or_a_d(void)
{
    #ifdef DUMP
    printf("OR A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif

    a |= d;
    f = a ? 0 : FLAG_ZERO;
}

static void or_a_e(void)
{
    #ifdef DUMP
    printf("OR A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif

    a |= e;
    f = a ? 0 : FLAG_ZERO;
}

static void or_a_h(void)
{
    #ifdef DUMP
    printf("OR A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif

    a |= h;
    f = a ? 0 : FLAG_ZERO;
}

static void or_a_l(void)
{
    #ifdef DUMP
    printf("OR A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif

    a |= l;
    f = a ? 0 : FLAG_ZERO;
}

static void or_a__hl(void)
{
    #ifdef DUMP
    printf("OR A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif

    a |= mem_readb(hl);
    f = a ? 0 : FLAG_ZERO;
}

static void or_a_a(void)
{
    #ifdef DUMP
    printf("OR A, A: A == 0x%02X\n", (unsigned)a);
    #endif

    f = a ? 0 : FLAG_ZERO;
}

static void cp_a_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(b));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void cp_a_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(c));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void cp_a_d(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, D: A == 0x%02X; D == 0x%02X\n", (unsigned)a, (unsigned)d);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(d));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void cp_a_e(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, E: A == 0x%02X; E == 0x%02X\n", (unsigned)a, (unsigned)e);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(e));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void cp_a_h(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, H: A == 0x%02X; H == 0x%02X\n", (unsigned)a, (unsigned)h);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(h));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void cp_a_l(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, L: A == 0x%02X; L == 0x%02X\n", (unsigned)a, (unsigned)l);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(l));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void cp_a__hl(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(mem_readb(hl)));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void cp_a_a(void)
{
    #ifdef DUMP
    printf("CP A, A: A == 0x%02X\n", (unsigned)a);
    #endif

    f = FLAG_SUB | FLAG_ZERO;
}

static void retnz(void)
{
    if (!(f & FLAG_ZERO))
    {
        #ifdef DUMP
        printf("RETNZ, returning\n");
        #endif
        ret();
    }
    else
    {
        #ifdef DUMP
        printf("RETNZ, staying\n");
        #endif
    }
}

static void pop_bc(void)
{
    #ifdef DUMP
    printf("POP BC: BC == 0x%04X\n", bc);
    #endif
    bc = pop();
}

static void jpnz(void)
{
    if (!(f & FLAG_ZERO))
    {
        #ifdef DUMP
        printf("JPNZ, branching\n");
        #endif
        jp();
    }
    else
    {
        #ifdef DUMP
        printf("JPNZ, staying\n");
        #endif
        ip += 2;
    }
}

static void jp(void)
{
    #ifdef DUMP
    printf("JP 0x%04X: IP == 0x%04X\n", (unsigned)nn, (unsigned)(ip - 1));
    #endif
    ip = nn;
}

static void callnz(void)
{
    if (!(f & FLAG_ZERO))
    {
        #ifdef DUMP
        printf("CALLNZ, calling\n");
        #endif
        call();
    }
    else
    {
        #ifdef DUMP
        printf("CALLNZ, staying\n");
        #endif
        ip += 2;
    }
}

static void push_bc(void)
{
    #ifdef DUMP
    printf("PUSH BC: BC == 0x%04X\n", bc);
    #endif
    push(bc);
}

static void add_a_s(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADD A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif
    __asm__ __volatile__ ("add al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(ip++)));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void rst0x00(void)
{
    #ifdef DUMP
    printf("RST $00: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0000;
}

static void retz(void)
{
    if (f & FLAG_ZERO)
    {
        #ifdef DUMP
        printf("RETZ, returning\n");
        #endif
        ret();
    }
    else
    {
        #ifdef DUMP
        printf("RETZ, staying\n");
        #endif
    }
}

static void ret(void)
{
    #ifdef DUMP
    uint16_t old_ip = ip;
    #endif
    ip = pop();
    #ifdef DUMP
    printf("RET: 0x%04X → 0x%04X\n", (unsigned)old_ip, (unsigned)ip);
    #endif
}

static void jpz(void)
{
    if (f & FLAG_ZERO)
    {
        #ifdef DUMP
        printf("JPZ, branching\n");
        #endif
        jp();
    }
    else
    {
        #ifdef DUMP
        printf("JPZ, staying\n");
        #endif
        ip += 2;
    }
}

static void prefix0xCB(void)
{
    int bval = 1 << ((mem_readb(ip) & 0x38) >> 3);
    int reg = mem_readb(ip) & 0x07;

    switch ((mem_readb(ip++) & 0xC0) >> 6)
    {
        case 0x01: // BIT
            #ifdef DUMP
            printf("BIT %i, ", (mem_readb(ip - 1) & 0x38) >> 3);
            #endif
            f &= FLAG_CRY;
            switch (reg)
            {
                case 0:
                    #ifdef DUMP
                    printf("B: B == 0x%02X\n", (unsigned)b);
                    #endif
                    f |= (b & bval) ? 0 : FLAG_ZERO;
                    break;
                case 1:
                    #ifdef DUMP
                    printf("C: C == 0x%02X\n", (unsigned)c);
                    #endif
                    f |= (c & bval) ? 0 : FLAG_ZERO;
                    break;
                case 2:
                    #ifdef DUMP
                    printf("D: D == 0x%02X\n", (unsigned)d);
                    #endif
                    f |= (d & bval) ? 0 : FLAG_ZERO;
                    break;
                case 3:
                    #ifdef DUMP
                    printf("E: E == 0x%02X\n", (unsigned)e);
                    #endif
                    f |= (e & bval) ? 0 : FLAG_ZERO;
                    break;
                case 4:
                    #ifdef DUMP
                    printf("H: H == 0x%02X\n", (unsigned)h);
                    #endif
                    f |= (h & bval) ? 0 : FLAG_ZERO;
                    break;
                case 5:
                    #ifdef DUMP
                    printf("L: L == 0x%02X\n", (unsigned)l);
                    #endif
                    f |= (l & bval) ? 0 : FLAG_ZERO;
                    break;
                case 6:
                    #ifdef DUMP
                    printf("(HL): HL == 0x%04X\n", (unsigned)hl);
                    #endif
                    f |= (mem_readb(hl) & bval) ? 0 : FLAG_ZERO;
                    break;
                case 7:
                    #ifdef DUMP
                    printf("A: A == 0x%02X\n", (unsigned)a);
                    #endif
                    f |= (a & bval) ? 0 : FLAG_ZERO;
                    break;
            }
            break;
        case 0x02: // RES
            #ifdef DUMP
            printf("RES %i, ", (mem_readb(ip - 1) & 0x38) >> 3);
            #endif
            switch (reg)
            {
                case 0:
                    #ifdef DUMP
                    printf("B: B == 0x%02X\n", (unsigned)b);
                    #endif
                    b &= ~bval;
                    break;
                case 1:
                    #ifdef DUMP
                    printf("C: C == 0x%02X\n", (unsigned)c);
                    #endif
                    c &= ~bval;
                    break;
                case 2:
                    #ifdef DUMP
                    printf("D: D == 0x%02X\n", (unsigned)d);
                    #endif
                    d &= ~bval;
                    break;
                case 3:
                    #ifdef DUMP
                    printf("E: E == 0x%02X\n", (unsigned)e);
                    #endif
                    e &= ~bval;
                    break;
                case 4:
                    #ifdef DUMP
                    printf("H: H == 0x%02X\n", (unsigned)h);
                    #endif
                    h &= ~bval;
                    break;
                case 5:
                    #ifdef DUMP
                    printf("L: L == 0x%02X\n", (unsigned)l);
                    #endif
                    l &= ~bval;
                    break;
                case 6:
                    #ifdef DUMP
                    printf("(HL): HL == 0x%04X\n", (unsigned)hl);
                    #endif
                    mem_writeb(hl, mem_readb(hl) & ~bval);
                    break;
                case 7:
                    #ifdef DUMP
                    printf("A: A == 0x%02X\n", (unsigned)a);
                    #endif
                    a &= ~bval;
                    break;
            }
            break;
        case 0x03: // SET
            #ifdef DUMP
            printf("SET %i, ", (mem_readb(ip - 1) & 0x38) >> 3);
            #endif
            switch (reg)
            {
                case 0:
                    #ifdef DUMP
                    printf("B: B == 0x%02X\n", (unsigned)b);
                    #endif
                    b |= bval;
                    break;
                case 1:
                    #ifdef DUMP
                    printf("C: C == 0x%02X\n", (unsigned)c);
                    #endif
                    c |= bval;
                    break;
                case 2:
                    #ifdef DUMP
                    printf("D: D == 0x%02X\n", (unsigned)d);
                    #endif
                    d |= bval;
                    break;
                case 3:
                    #ifdef DUMP
                    printf("E: E == 0x%02X\n", (unsigned)e);
                    #endif
                    e |= bval;
                    break;
                case 4:
                    #ifdef DUMP
                    printf("H: H == 0x%02X\n", (unsigned)h);
                    #endif
                    h |= bval;
                    break;
                case 5:
                    #ifdef DUMP
                    printf("L: L == 0x%02X\n", (unsigned)l);
                    #endif
                    l |= bval;
                    break;
                case 6:
                    #ifdef DUMP
                    printf("(HL): HL == 0x%04X\n", (unsigned)hl);
                    #endif
                    mem_writeb(hl, mem_readb(hl) | bval);
                    break;
                case 7:
                    #ifdef DUMP
                    printf("A: A == 0x%02X\n", (unsigned)a);
                    #endif
                    a |= bval;
                    break;
            }
            break;
        default:
            if (handle0xCB[(int)mem_readb(ip - 1)] == NULL)
            {
                printf("Unknown opcode 0x%02X, prefixed by 0xCB\n", (unsigned)mem_readb(ip - 1));
                exit(1);
            }

            handle0xCB[(int)mem_readb(ip - 1)]();
    }
}

static void callz(void)
{
    if (f & FLAG_ZERO)
    {
        #ifdef DUMP
        printf("CALLZ, calling\n");
        #endif
        call();
    }
    else
    {
        #ifdef DUMP
        printf("CALLZ, staying\n");
        #endif
        ip += 2;
    }
}

static void call(void)
{
    #ifdef DUMP
    printf("CALL 0x%04X: IP == 0x%04X\n", (unsigned)nn, (unsigned)(ip - 1));
    #endif
    push(ip + 2);
    ip = nn;
}

static void adc_a_s(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("ADC A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(ip++)));
    else
        __asm__ __volatile__ ("clc; adc al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(ip++)));

    f = 0;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
}

static void rst0x08(void)
{
    #ifdef DUMP
    printf("RST $08: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0008;
}

static void retnc(void)
{
    if (!(f & FLAG_CRY))
    {
        #ifdef DUMP
        printf("RETNC, returning\n");
        #endif
        ret();
    }
    else
    {
        #ifdef DUMP
        printf("RETNC, staying\n");
        #endif
    }
}

static void pop_de(void)
{
    #ifdef DUMP
    printf("POP DE: DE == 0x%04X\n", (unsigned)de);
    #endif
    de = pop();
}

static void jpnc(void)
{
    if (!(f & FLAG_CRY))
    {
        #ifdef DUMP
        printf("JPNC, branching\n");
        #endif
        jp();
    }
    else
    {
        #ifdef DUMP
        printf("JPNC, staying\n");
        #endif
        ip += 2;
    }
}

static void callnc(void)
{
    if (!(f & FLAG_CRY))
    {
        #ifdef DUMP
        printf("CALLNC, calling\n");
        #endif
        call();
    }
    else
    {
        #ifdef DUMP
        printf("CALLNC, staying\n");
        #endif
        ip += 2;
    }
}

static void push_de(void)
{
    #ifdef DUMP
    printf("PUSH DE: DE == 0x%04X\n", (unsigned)de);
    #endif
    push(de);
}

static void sub_a_s(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(ip++)));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ret0x10(void)
{
    #ifdef DUMP
    printf("RST $10: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0010;
}

static void retc(void)
{
    if (f & FLAG_CRY)
    {
        #ifdef DUMP
        printf("RETC, returning\n");
        #endif
        ret();
    }
    else
    {
        #ifdef DUMP
        printf("RETC, staying\n");
        #endif
    }
}

static void reti(void)
{
    #ifdef DUMP
    printf("RETI\n");
    #endif

    want_ints_to_be = 1;
    ints_enabled = 1;
    ret();
}

static void jpc(void)
{
    if (f & FLAG_CRY)
    {
        #ifdef DUMP
        printf("JPC, branching\n");
        #endif
        jp();
    }
    else
    {
        #ifdef DUMP
        printf("JPC, staying\n");
        #endif
        ip += 2;
    }
}

static void callc(void)
{
    if (f & FLAG_CRY)
    {
        #ifdef DUMP
        printf("CALLC, calling\n");
        #endif
        call();
    }
    else
    {
        #ifdef DUMP
        printf("CALLC, staying\n");
        #endif
        ip += 2;
    }
}

static void sbc_a_s(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SBC A, 0x%02X: A == 0x%02X\n", (unsigned)a, (unsigned)mem_readb(ip));
    #endif
    if (f & FLAG_CRY)
        __asm__ __volatile__ ("stc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(ip++)));
    else
        __asm__ __volatile__ ("clc; sbb al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "d"(mem_readb(ip++)));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void rst0x18(void)
{
    #ifdef DUMP
    printf("RST $18: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0018;
}

static void ld__ffn_a(void)
{
    #ifdef DUMP
    printf("LD ($FF00 + 0x%02X), A: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif
    mem_writeb(0xFF00 + mem_readb(ip++), a);
}

static void pop_hl(void)
{
    #ifdef DUMP
    printf("POP HL: HL == 0x%04X\n", (unsigned)hl);
    #endif
    hl = pop();
}

static void ld__ffc_a(void)
{
    #ifdef DUMP
    printf("LD ($FF00 + C), A: C == 0x%02X; A == 0x%02X\n", (unsigned)c, (unsigned)a);
    #endif
    mem_writeb(0xFF00 + c, a);
}

static void push_hl(void)
{
    #ifdef DUMP
    printf("PUSH HL: HL == 0x%04X\n", (unsigned)hl);
    #endif
    push(hl);
}

static void and_a_s(void)
{
    #ifdef DUMP
    printf("AND A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif

    a &= mem_readb(ip++);
    if (a)
        f = FLAG_HCRY;
    else
        f = FLAG_HCRY | FLAG_ZERO;
}

static void rst0x20(void)
{
    #ifdef DUMP
    printf("RST $20: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0020;
}

static void add_sp_s(void)
{
    int add = (int)(signed char)mem_readb(ip++);
    int result = sp + add;

    #ifdef DUMP
    printf("ADD SP, %i: SP == 0x%04X\n", add, sp);
    #endif

    f = 0;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if ((sp & 0xFFF) + (add & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    sp = result;
}

static void jp__hl(void)
{
    #ifdef DUMP
    printf("JP (HL): HL == 0x%04X; IP == 0x%04X\n", (unsigned)hl, (unsigned)ip);
    #endif
    ip = hl;
}

static void ld__nn_a(void)
{
    #ifdef DUMP
    printf("LD (0x%04X), A: A == 0x%02X\n", (unsigned)nn, (unsigned)a);
    #endif
    mem_writeb(nn, a);
    ip += 2;
}

static void xor_a_s(void)
{
    #ifdef DUMP
    printf("XOR A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif

    a ^= mem_readb(ip++);
    f = a ? 0 : FLAG_ZERO;
}

static void rst0x28(void)
{
    #ifdef DUMP
    printf("RST $28: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0028;
}

static void ld_a__ffn(void)
{
    #ifdef DUMP
    printf("LD A, ($FF00 + 0x%02X): A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif
    a = mem_readb(0xFF00 + (unsigned)mem_readb(ip++));
}

static void pop_af(void)
{
    #ifdef DUMP
    printf("POP AF: AF == 0x%04X\n", af);
    #endif
    af = pop();
}

static void ld_a__ffc(void)
{
    #ifdef DUMP
    printf("LD A, ($FF00 + C): A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif
    a = mem_readb(0xFF00 + (unsigned)c);
}

static void di_(void)
{
    #ifdef DUMP
    printf("DI: Interrupts %s\n", ints_enabled ? "enabled" : "disabled");
    #endif
    want_ints_to_be = 0;
}

static void push_af(void)
{
    #ifdef DUMP
    printf("PUSH AF: AF == 0x%04X\n", (unsigned)af);
    #endif
    push(af);
}

static void or_a_s(void)
{
    #ifdef DUMP
    printf("OR A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif

    a |= mem_readb(ip++);
    f = a ? 0 : FLAG_ZERO;
}

static void rst0x30(void)
{
    #ifdef DUMP
    printf("RST $30: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0030;
}

static void ld_hl_spn(void)
{
    int result;

    #ifdef DUMP
    printf("LD HL, SP + %i: HL == 0x%04X; SP == 0x%04X\n", (int)(signed char)mem_readb(ip), (unsigned)hl, (unsigned)sp);
    #endif

    result = sp + (int)(signed char)mem_readb(ip++);

    f = 0;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if ((hl & 0xFFF) + (bc & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    hl = result;
}

static void ld_sp_hl(void)
{
    #ifdef DUMP
    printf("LD SP, HL: SP == 0x%04X; HL == 0x%04X\n", (unsigned)sp, (unsigned)hl);
    #endif
    sp = hl;
}

static void ld_a__nn(void)
{
    #ifdef DUMP
    printf("LD A, (0x%04X): A == 0x%02X\n", (unsigned)nn, (unsigned)a);
    #endif
    a = mem_readb(nn);
    ip += 2;
}

static void ei_(void)
{
    #ifdef DUMP
    printf("EI: Interrupts %s\n", ints_enabled ? "enabled" : "disabled");
    #endif
    want_ints_to_be = 1;
}

static void cp_s(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("CP A, 0x%02X: A == 0x%02X\n", (unsigned)mem_readb(ip), (unsigned)a);
    #endif

    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,dl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "d"(mem_readb(ip++)));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void rst0x38(void)
{
    #ifdef DUMP
    printf("RST $38: IP == 0x%04X\n", (unsigned)ip - 1);
    #endif
    #ifdef HALT_ON_RST
    printf("RST – halting\n");
    exit(1);
    #endif
    push(ip);
    ip = 0x0038;
}



static void (*const handle[256])(void) =
{
    &nop,           // 0x00
    &ld_bc_nn,
    &ld__bc_a,
    &inc_bc,
    &inc_b,
    &dec_b,
    &ld_b_n,
    &rlca,
    &ld__nn_sp,     // 0x08
    &add_hl_bc,
    &ld_a__bc,
    &dec_bc,
    &inc_c,
    &dec_c,
    &ld_c_n,
    &rrca,
    &prefix0x10,    // 0x10
    &ld_de_nn,
    &ld__de_a,
    &inc_de,
    &inc_d,
    &dec_d,
    &ld_d_n,
    &rla,
    &jr,            // 0x18
    &add_hl_de,
    &ld_a__de,
    &dec_de,
    &inc_e,
    &dec_e,
    &ld_e_n,
    &rra,
    &jrnz,          // 0x20
    &ld_hl_nn,
    &ldi__hl_a,
    &inc_hl,
    &inc_h,
    &dec_h,
    &ld_h_n,
    &daa,
    &jrz,           // 0x28
    &add_hl_hl,
    &ldi_a__hl,
    &dec_hl,
    &inc_l,
    &dec_l,
    &ld_l_n,
    &cpl_a,
    &jrnc,          // 0x30
    &ld_sp_nn,
    &ldd__hl_a,
    &inc_sp,
    &inc__hl,
    &dec__hl,
    &ld__hl_n,
    &scf,
    &jrc,           // 0x38
    &add_hl_sp,
    &ldd_a__hl,
    &dec_sp,
    &inc_a,
    &dec_a,
    &ld_a_s,
    &ccf,
    &ld_b_b,        // 0x40
    &ld_b_c,
    &ld_b_d,
    &ld_b_e,
    &ld_b_h,
    &ld_b_l,
    &ld_b__hl,
    &ld_b_a,
    &ld_c_b,        // 0x48
    &ld_c_c,
    &ld_c_d,
    &ld_c_e,
    &ld_c_h,
    &ld_c_l,
    &ld_c__hl,
    &ld_c_a,
    &ld_d_b,        // 0x50
    &ld_d_c,
    &ld_d_d,
    &ld_d_e,
    &ld_d_h,
    &ld_d_l,
    &ld_d__hl,
    &ld_d_a,
    &ld_e_b,        // 0x58
    &ld_e_c,
    &ld_e_d,
    &ld_e_e,
    &ld_e_h,
    &ld_e_l,
    &ld_e__hl,
    &ld_e_a,
    &ld_h_b,        // 0x60
    &ld_h_c,
    &ld_h_d,
    &ld_h_e,
    &ld_h_h,
    &ld_h_l,
    &ld_h__hl,
    &ld_h_a,
    &ld_l_b,        // 0x68
    &ld_l_c,
    &ld_l_d,
    &ld_l_e,
    &ld_l_h,
    &ld_l_l,
    &ld_l__hl,
    &ld_l_a,
    &ld__hl_b,      // 0x70
    &ld__hl_c,
    &ld__hl_d,
    &ld__hl_e,
    &ld__hl_h,
    &ld__hl_l,
    &halt,
    &ld__hl_a,
    &ld_a_b,        // 0x78
    &ld_a_c,
    &ld_a_d,
    &ld_a_e,
    &ld_a_h,
    &ld_a_l,
    &ld_a__hl,
    &ld_a_a,
    &add_a_b,       // 0x80
    &add_a_c,
    &add_a_d,
    &add_a_e,
    &add_a_h,
    &add_a_l,
    &add_a__hl,
    &add_a_a,
    &adc_a_b,       // 0x88
    &adc_a_c,
    &adc_a_d,
    &adc_a_e,
    &adc_a_h,
    &adc_a_l,
    &adc_a__hl,
    &adc_a_a,
    &sub_a_b,       // 0x90
    &sub_a_c,
    &sub_a_d,
    &sub_a_e,
    &sub_a_h,
    &sub_a_l,
    &sub_a__hl,
    &sub_a_a,
    &sbc_a_b,       // 0x98
    &sbc_a_c,
    &sbc_a_d,
    &sbc_a_e,
    &sbc_a_h,
    &sbc_a_l,
    &sbc_a__hl,
    &sbc_a_a,
    &and_a_b,       // 0xA0
    &and_a_c,
    &and_a_d,
    &and_a_e,
    &and_a_h,
    &and_a_l,
    &and_a__hl,
    &and_a_a,
    &xor_a_b,       // 0xA8
    &xor_a_c,
    &xor_a_d,
    &xor_a_e,
    &xor_a_h,
    &xor_a_l,
    &xor_a__hl,
    &xor_a_a,
    &or_a_b,        // 0xB0
    &or_a_c,
    &or_a_d,
    &or_a_e,
    &or_a_h,
    &or_a_l,
    &or_a__hl,
    &or_a_a,
    &cp_a_b,        // 0xB8
    &cp_a_c,
    &cp_a_d,
    &cp_a_e,
    &cp_a_h,
    &cp_a_l,
    &cp_a__hl,
    &cp_a_a,
    &retnz,         // 0xC0
    &pop_bc,
    &jpnz,
    &jp,
    &callnz,
    &push_bc,
    &add_a_s,
    &rst0x00,
    &retz,          // 0xC8
    &ret,
    &jpz,
    &prefix0xCB,
    &callz,
    &call,
    &adc_a_s,
    &rst0x08,
    &retnc,         // 0xD0
    &pop_de,
    &jpnc,
    NULL,
    &callnc,
    &push_de,
    &sub_a_s,
    &ret0x10,
    &retc,          // 0xD8
    &reti,
    &jpc,
    NULL,
    &callc,
    NULL,
    &sbc_a_s,
    &rst0x18,
    &ld__ffn_a,     // 0xE0
    &pop_hl,
    &ld__ffc_a,
    NULL,
    NULL,
    &push_hl,
    &and_a_s,
    &rst0x20,
    &add_sp_s,      // 0xE8
    &jp__hl,
    &ld__nn_a,
    NULL,
    NULL,
    NULL,
    &xor_a_s,
    &rst0x28,
    &ld_a__ffn,     // 0xF0
    &pop_af,
    &ld_a__ffc,
    &di_,
    NULL,
    &push_af,
    &or_a_s,
    &rst0x30,
    &ld_hl_spn,     // 0xF8
    &ld_sp_hl,
    &ld_a__nn,
    &ei_,
    NULL,
    NULL,
    &cp_s,
    &rst0x38        // 0xFF
};

void run(void)
{
    #ifdef TRUE_TIMING
    int64_t too_short = 0, collect_sleep_time = 0;
    #endif

    init_video();

    ip = 0x0100;
    sp = 0xFFFE;
    af = 0x11B0;
    bc = 0x0000;
    de = 0xFF56;
    hl = 0x000D;

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
    io_regs->stat = 1;
    io_regs->scy = 0x00;
    io_regs->scx = 0x00;
    io_regs->lyc = 0x00;
    io_regs->bgp = 0xFC;
    io_regs->obp0 = 0xFF;
    io_regs->obp1 = 0xFF;
    io_regs->wy = 0x00;
    io_regs->wx = 0x00;
    io_regs->int_enable = 0x00;

    btm[0] = (uint8_t *)&full_vidram[0x1800];
    btm[1] = (uint8_t *)&full_vidram[0x3800];
    bwtd[0] = (uint8_t *)&full_vidram[0x0000];
    bwtd[1] = (uint8_t *)&full_vidram[0x2000];
    wtm = (uint8_t *)&vidram[0x1800];

    for(int i = 0; i < 32; i++)
    {
        bpalette[i] = 0x011F;
        opalette[i] = 0x011F;
    }

    #ifdef TRUE_TIMING
    last_tsc = rdtsc();
    #endif

    for (;;)
    {
        int change_int_status = 0, cyc, opcode;

        if (want_ints_to_be != ints_enabled)
            change_int_status = 1;

        opcode = mem_readb(ip);
        if (handle[opcode] == NULL)
        {
            printf("Unknown opcode 0x%02X\n", opcode);
            break;
        }

        #ifdef DUMP_REGS
        printf("IP=%04x AF=%04x BC=%04x DE=%04x HL=%04x SP=%04x I=%04x\n", (unsigned)ip, (unsigned)af, (unsigned)bc, (unsigned)de, (unsigned)hl, (unsigned)sp, (unsigned)io_regs->int_flag);
        #endif

        #ifdef DUMP
        printf("0x%04X — ", (unsigned)ip);
        #endif
        ip++;

        if (opcode != 0xCB)
            cyc = cycles[opcode];
        else
            cyc = cycles0xCB[(int)mem_readb(ip)];

        handle[opcode]();

        if (change_int_status)
        {
            #ifdef DUMP
            printf("Changing interrupt status from %s to %s\n", ints_enabled ? "enabled" : "disabled", want_ints_to_be ? "enabled" : "disabled");
            #endif
            ints_enabled = want_ints_to_be;
            change_int_status = 0;
        }

        update_timer(cyc);
        generate_interrupts();

        #ifdef TRUE_TIMING
        new_tsc = rdtsc();
        diff = new_tsc - last_tsc;
        last_tsc = new_tsc;

        too_short = cyc - 1000LL * (uint64_t)diff / (uint64_t)rdtsc_resolution;
        collect_sleep_time += too_short;

        if (collect_sleep_time >= 10000)
        {
            nanosleep(&(struct timespec){ .tv_nsec = collect_sleep_time * 1000 }, NULL);
            collect_sleep_time = 0;
        }
        #endif
    }
}
