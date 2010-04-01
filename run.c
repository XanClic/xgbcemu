#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "gbc.h"

#define DUMP

#define X86_ZF (1 << 6)
#define X86_CF (1 << 0)
#define X86_AF (1 << 4)

#define nn *((uint16_t *)&memory[ip])

static void (*handle0xCB[256])(void) = { NULL };

static void mem_writeb(uintptr_t addr, uint8_t value)
{
    uint8_t *caddr = (uint8_t *)&memory[addr];

    #ifdef DUMP
    printf("0x%02X -> 0x%04X\n", value, addr);
    #endif

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
        io_outb((uint8_t)(addr - 0xFF00), value);
    else
        *caddr = value;
}

static void mem_writew(uintptr_t addr, uint16_t value)
{
    uint16_t *caddr = (uint16_t *)&memory[addr];

    #ifdef DUMP
    printf("0x%04X -> 0x%04X\n", value, addr);
    #endif

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
        printf("I/O write: 0x%04X -> 0x%04X (16 bit not supported!)\n", (unsigned)value, addr);
        exit(0);
    }
    else
        *caddr = value;
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC B: B == 0x%02X\n", (unsigned)b);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(b), "=d"(eflags) : "a"(b));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC B: B == 0x%02X\n", (unsigned)b);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(b), "=d"(eflags) : "a"(b));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld_b_n(void)
{
    #ifdef DUMP
    printf("LD B, 0x%02X: B == 0x%02X\n", (unsigned)memory[ip], (unsigned)b);
    #endif
    b = memory[ip++];
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

    f = 0;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if (!result)
        f |= FLAG_ZERO;
    if ((hl & 0xFFF) + (bc & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    hl = result;
}

static void ld_a__bc(void)
{
    #ifdef DUMP
    printf("LD A, (BC): A == 0x%02X; BC == 0x%04X\n", (unsigned)a, (unsigned)bc);
    #endif
    a = memory[bc];
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC C: C == 0x%02X\n", (unsigned)c);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(c), "=d"(eflags) : "a"(c));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC C: C == 0x%02X\n", (unsigned)c);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(c), "=d"(eflags) : "a"(c));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld_c_n(void)
{
    #ifdef DUMP
    printf("LD C, 0x%02X: C == 0x%02X\n", (unsigned)memory[ip], (unsigned)c);
    #endif
    c = memory[ip++];
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
    switch (memory[ip])
    {
        case 0x00:
            printf("STOP\n");
            exit(0);
        default:
            printf("Unknown opcode 0x%02X, prefixed by 0x10.\n", (unsigned)memory[ip]);
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC D: D == 0x%02X\n", (unsigned)d);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(d), "=d"(eflags) : "a"(d));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec_d(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC D: D == 0x%02X\n", (unsigned)d);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(d), "=d"(eflags) : "a"(d));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld_d_n(void)
{
    #ifdef DUMP
    printf("LD D, 0x%02X: D == 0x%02X\n", (unsigned)memory[ip], (unsigned)d);
    #endif
    d = memory[ip++];
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
    int off = ((signed char *)memory)[ip];
    printf("JR %i: 0x%04X → 0x%04X\n", (int)off, (unsigned)(ip - 1), (unsigned)(ip + off + 1));
    #endif
    ip += ((signed char *)memory)[ip] + 1;
}

static void add_hl_de(void)
{
    int result = hl + de;

    #ifdef DUMP
    printf("ADD HL, DE: HL == 0x%04X; DE == 0x%04X\n", (unsigned)hl, (unsigned)de);
    #endif

    f = 0;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if (!result)
        f |= FLAG_ZERO;
    if ((hl & 0xFFF) + (bc & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    hl = result;
}

static  void ld_a__de(void)
{
    #ifdef DUMP
    printf("LD A, (DE): A == 0x%02X; DE == 0x%04X\n", (unsigned)a, (unsigned)de);
    #endif
    a = memory[de];
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC E: E == 0x%02X\n", (unsigned)e);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(e), "=d"(eflags) : "a"(e));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec_e(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC E: E == 0x%02X\n", (unsigned)e);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(e), "=d"(eflags) : "a"(e));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld_e_n(void)
{
    #ifdef DUMP
    printf("LD E, 0x%02X: E == 0x%02X\n", (unsigned)memory[ip], (unsigned)e);
    #endif
    e = memory[ip++];
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC H: H == 0x%02X\n", (unsigned)h);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(h), "=d"(eflags) : "a"(h));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec_h(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC H: H == 0x%02X\n", (unsigned)h);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(h), "=d"(eflags) : "a"(h));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld_h_n(void)
{
    #ifdef DUMP
    printf("LD H, 0x%02X: H == 0x%02X\n", memory[ip], h);
    #endif
    h = memory[ip++];
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

    f = 0;
    if (hl & 0x8000)
        f |= FLAG_CRY;
    if (hl & 0x0800)
        f |= FLAG_HCRY;
    if (!hl)
        f |= FLAG_ZERO;

    hl <<= 1;
}

static void ldi_a__hl(void)
{
    #ifdef DUMP
    printf("LDI A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    a = memory[hl++];
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC L: L == 0x%02X\n", (unsigned)l);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(l), "=d"(eflags) : "a"(l));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec_l(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC L: L == 0x%02X\n", (unsigned)l);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(l), "=d"(eflags) : "a"(l));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld_l_n(void)
{
    #ifdef DUMP
    printf("LD L, 0x%02X: L == 0x%02X\n", (unsigned)memory[ip], (unsigned)l);
    #endif
    l = memory[ip++];
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif
    __asm__ __volatile__ ("inc byte ptr [eax]; pushfd; pop eax" : "=a"(eflags) : "a"(&memory[hl]));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec__hl(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif
    __asm__ __volatile__ ("dec byte ptr [eax]; pushfd; pop eax" : "=a"(eflags) : "a"(&memory[hl]));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld__hl_n(void)
{
    #ifdef DUMP
    printf("LD (HL), 0x%02X: HL == 0x%04X\n", (unsigned)memory[ip], (unsigned)hl);
    #endif
    mem_writeb(hl, memory[ip++]);
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

    f = 0;
    if (result & ~0xFFFF)
        f |= FLAG_CRY;
    result &= 0xFFFF;
    if (!result)
        f |= FLAG_ZERO;
    if ((hl & 0xFFF) + (bc & 0xFFF) > 0xFFF)
        f |= FLAG_HCRY;

    hl = result;
}

static void ldd_a__hl(void)
{
    #ifdef DUMP
    printf("LDD A, (HL): A == 0x%02X; HL == 0x%04X\n", (unsigned)a, (unsigned)hl);
    #endif
    a = memory[hl--];
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
    uint32_t eflags;

    #ifdef DUMP
    printf("INC A: A == 0x%02X\n", (unsigned)a);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void dec_a(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("DEC A: A == 0x%02X\n", (unsigned)a);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void ld_a_s(void)
{
    #ifdef DUMP
    printf("LD A, 0x%02X: A == 0x%02X\n", (unsigned)memory[ip], (unsigned)a);
    #endif
    a = memory[ip++];
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
    b = memory[hl];
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
    c = memory[hl];
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
    d = memory[hl];
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
    e = memory[hl];
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
    h = memory[hl];
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
    l = memory[hl];
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
    interrupt_issued = 0;
    while (!interrupt_issued);
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
    a = memory[hl];
}

static void ld_a_a(void)
{
    #ifdef DUMP
    printf("LD A, A: A == 0x%02X\n", (unsigned)a);
    #endif
}

static void sub_a_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, B: A == 0x%02X; B == 0x%02X\n", (unsigned)a, (unsigned)b);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "b"(b));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
}

static void xor_a_a(void)
{
    #ifdef DUMP
    printf("XOR A, A: A == 0x%02X\n", (unsigned)a);
    #endif
    a = 0;
    f = FLAG_ZERO;
}

static void or_a_c(void)
{
    #ifdef DUMP
    printf("OR A, C: A == 0x%02X; C == 0x%02X\n", (unsigned)a, (unsigned)c);
    #endif
    a |= c;
    if (!a)
        f = FLAG_ZERO;
    else
        f = 0;
}

static void jp(void)
{
    #ifdef DUMP
    printf("JP 0x%04X: IP == 0x%04X\n", (unsigned)nn, (unsigned)(ip - 1));
    #endif
    ip = nn;
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

static void prefix0xCB(void)
{
    if (handle0xCB[(int)memory[ip]] == NULL)
    {
        printf("Unknown opcode 0x%02X, prefixed by 0xCB\n", (unsigned)memory[ip]);
        exit(1);
    }

    ip++;
    handle0xCB[(int)memory[ip - 1]]();
}

static void call(void)
{
    #ifdef DUMP
    printf("CALL 0x%04X: IP == 0x%04X\n", (unsigned)nn, (unsigned)(ip - 1));
    #endif
    push(ip + 2);
    ip = nn;
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

static void ld__ffn_a(void)
{
    #ifdef DUMP
    printf("LD ($FF00 + 0x%02X), A: A == 0x%02X\n", (unsigned)memory[ip], (unsigned)a);
    #endif
    mem_writeb(0xFF00 + memory[ip++], a);
}

static void ld__nn_a(void)
{
    #ifdef DUMP
    printf("LD (0x%04X), A: A == 0x%02X\n", (unsigned)nn, (unsigned)a);
    #endif
    mem_writeb(nn, a);
    ip += 2;
}

static void ld_a__ffn(void)
{
    #ifdef DUMP
    printf("LD A, ($FF00 + 0x%02X): A == 0x%02X\n", (unsigned)memory[ip], (unsigned)a);
    #endif
    a = memory[0xFF00 + memory[ip++]];
}

static void di_(void)
{
    #ifdef DUMP
    printf("DI: Interrupts %s\n", ints_enabled ? "enabled" : "disabled");
    #endif
    want_ints_to_be = 0;
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
    printf("CP A, 0x%02X: A == 0x%02X\n", (unsigned)memory[ip], (unsigned)a);
    #endif
    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,bl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "b"(memory[ip++]));
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
    push(ip);
    ip = 0x0038;
}

static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (uint64_t)lo | ((uint64_t)hi << 32);
}

static void res_b_a(void)
{
    #ifdef DUMP
    printf("RES %i, A: A == 0x%02X\n", (int)memory[ip], (unsigned)a);
    #endif
    a &= ~(1 << memory[ip++]);
}



static void (*handle[256])(void) =
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
    NULL,           // 0x80
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0x88
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &sub_a_b,       // 0x90
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0x98
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xA0
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xA8
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &xor_a_a,
    NULL,           // 0xB0
    &or_a_c,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xB8
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xC0
    NULL,
    NULL,
    &jp,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xC8
    &ret,
    NULL,
    &prefix0xCB,
    NULL,
    &call,
    NULL,
    NULL,
    &retnc,         // 0xD0
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xD8
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &ld__ffn_a,     // 0xE0
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xE8
    NULL,
    &ld__nn_a,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &ld_a__ffn,     // 0xF0
    NULL,
    NULL,
    &di_,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,           // 0xF8
    NULL,
    NULL,
    &ei_,
    NULL,
    NULL,
    &cp_s,
    &rst0x38        // 0xFF
};

void run(void)
{
    uint64_t last_tsc;

    init_video();

    handle0xCB[0x87] = &res_b_a;

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

    last_tsc = rdtsc();

    for (;;)
    {
        int change_int_status = 0;

        if (want_ints_to_be != ints_enabled)
            change_int_status = 1;

        if (handle[(int)memory[ip]] == NULL)
        {
            printf("Unknown opcode 0x%02X\n", (unsigned)memory[ip]);
            break;
        }

        ip++;
        handle[(int)memory[ip - 1]]();

        if (change_int_status)
        {
            printf("Changing interrupt status from %s to %s\n", ints_enabled ? "enabled" : "disabled", want_ints_to_be ? "enabled" : "disabled");
            ints_enabled = want_ints_to_be;
            change_int_status = 0;
        }

        uint64_t new_tsc = rdtsc();
        uint32_t diff = new_tsc - last_tsc;
        last_tsc = new_tsc;

        update_timer(1000000LL * (uint64_t)diff / (uint64_t)rdtsc_resolution);
    }
}
