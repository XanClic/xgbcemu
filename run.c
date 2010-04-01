#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "gbc.h"

#define DUMP

#define X86_ZF (1 << 6)
#define X86_CF (1 << 0)
#define X86_AF (1 << 4)

static void (*handle[256])(void) = { NULL };
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
        printf("I/O write: 0x%04X -> 0x%04X\n", value, addr);
        exit(0);
    }
    else
        *caddr = value;
}

static void nop(void)
{
    #ifdef DUMP
    printf("Doing NOP\n");
    #endif
    ip++;
}

static void ld_bc_nn(void)
{
    ip++;
    #ifdef DUMP
    printf("LD BC, nn: Loading 0x%04X into BC (0x%04X)\n", *((uint16_t *)&memory[ip]), bc);
    #endif
    bc = *((uint16_t *)&memory[ip]);
    ip += 2;
}

static void ld__bc__a(void)
{
    #ifdef DUMP
    printf("LD (BC), A: Loading 0x%02X into 0x%04X\n", a, bc);
    #endif
    mem_writeb(bc, a);
    ip++;
}

static void inc_bc(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("Incrementing BC (will be 0x%04X)\n", (bc + 1) & 0xFFFF);
    #endif
    __asm__ __volatile__ ("inc ax; pushfd; pop edx" : "=a"(bc), "=d"(eflags) : "a"(bc));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;

    ip++;
}

static void inc_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("Incrementing B (will be 0x%02X)\n", (b + 1) & 0xFF);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(b), "=d"(eflags) : "a"(b));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;

    ip++;
}

static void dec_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("Decrementing B (will be 0x%02X)\n", (b - 1) & 0xFF);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(b), "=d"(eflags) : "a"(b));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;

    ip++;
}

static void ld_b_n(void)
{
    ip++;
    #ifdef DUMP
    printf("LD B, n: Loading 0x%02X into B (0x%02X)\n", memory[ip], b);
    #endif
    b = memory[ip++];
}

static void rlca(void)
{
    #ifdef DUMP
    printf("RLCA: A is 0x%02X, will be 0x%02X\n", a, ((a << 1) & 0xFF) | !!(a & 0x80));
    #endif
    f = (a & 0x80) ? FLAG_CRY : 0;
    __asm__ __volatile__ ("rol al,1" : "=a"(a) : "a"(a));
    if (!a)
        f |= FLAG_ZERO;
    ip++;
}

static void ld__nn__sp(void)
{
    ip++;
    #ifdef DUMP
    printf("LD (nn), SP: Loading 0x%04X into 0x%04X\n", sp, *((uint16_t *)&memory[ip]));
    #endif
    mem_writew(*((uint16_t *)&memory[ip]), sp);
    ip += 2;
}

static void add_hl_bc(void)
{
    int result = hl + bc;

    #ifdef DUMP
    printf("ADD HL, BC: 0x%04X + 0x%04X = 0x%04X\n", hl, bc, result & 0xFFFF);
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

    ip++;
}

static void ld_a__bc(void)
{
    #ifdef DUMP
    printf("LD A, (BC): Loading 0x%02X into A (0x%02X)\n", memory[bc], a);
    #endif

    a = memory[bc];
    ip++;
}

static void dec_bc(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("Decrementing BC (will be 0x%04X)\n", (bc - 1) & 0xFFFF);
    #endif
    __asm__ __volatile__ ("dec ax; pushfd; pop edx" : "=a"(bc), "=d"(eflags) : "a"(bc));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;

    ip++;
}

static void inc_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("Incrementing C (will be 0x%02X)\n", (c + 1) & 0xFF);
    #endif
    __asm__ __volatile__ ("inc al; pushfd; pop edx" : "=a"(c), "=d"(eflags) : "a"(c));

    f &= FLAG_CRY;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;

    ip++;
}

static void dec_c(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("Decrementing C (will be 0x%02X)\n", (c - 1) & 0xFF);
    #endif
    __asm__ __volatile__ ("dec al; pushfd; pop edx" : "=a"(c), "=d"(eflags) : "a"(c));

    f &= FLAG_CRY;
    f |= FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;

    ip++;
}

static void ld_c_n(void)
{
    ip++;
    #ifdef DUMP
    printf("LD C, n: Loading 0x%02X into C (0x%02X)\n", memory[ip], c);
    #endif
    c = memory[ip++];
}

static void rrca(void)
{
    #ifdef DUMP
    printf("RRCA: A is 0x%02X, will be 0x%02X\n", a, (a >> 1) | ((a & 0x01) * 0x80));
    #endif
    f = (a & 0x01) ? FLAG_CRY : 0;
    __asm__ __volatile__ ("ror al,1" : "=a"(a) : "a"(a));
    if (!a)
        f |= FLAG_ZERO;
    ip++;
}

static void prefix0x10(void)
{
    switch (memory[++ip])
    {
        case 0x00:
            printf("STOP called, exiting.\n");
            exit(0);
        default:
            printf("Unknown opcode 0x%02X, prefixed by 0x10.\n", memory[ip]);
            exit(1);
    }
}

static void jr(void)
{
    ip++;
    #ifdef DUMP
    int off = ((signed char *)memory)[ip];
    printf("Doing relative jump, offset %i (0x%04X → 0x%04X)\n", off, ip - 1, ip + off + 1);
    #endif
    ip += ((signed char *)memory)[ip] + 1;
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
        ip += 2;
    }
}

static void ld_hl_nn(void)
{
    ip++;
    #ifdef DUMP
    printf("LD HL, nn: Loading 0x%04X into HL (0x%04X)\n", *((uint16_t *)&memory[ip]), hl);
    #endif
    hl = *((uint16_t *)&memory[ip]);
    ip += 2;
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
        ip += 2;
    }
}

static void ld_sp_nn(void)
{
    ip++;
    #ifdef DUMP
    printf("LD SP, nn: Loading 0x%04X into SP (0x%04X)\n", *((uint16_t *)&memory[ip]), sp);
    #endif
    sp = *((uint16_t *)&memory[ip]);
    ip += 2;
}

static void inc_hl(void)
{
    #ifdef DUMP
    printf("Incrementing HL (will be 0x%04X)\n", hl + 1);
    #endif
    hl++;
    ip++;
}

static void ld__hl__n(void)
{
    ip++;
    #ifdef DUMP
    printf("LD (HL), n: Loading 0x%02X into 0x%04X\n", memory[ip], hl);
    #endif
    mem_writeb(hl, memory[ip++]);
}

static void ld_a_s(void)
{
    ip++;
    #ifdef DUMP
    printf("LD A, #: Loading 0x%02X into A (0x%02X)\n", memory[ip], a);
    #endif
    a = memory[ip++];
}

static void ld_b_a(void)
{
    #ifdef DUMP
    printf("LD B, A: Loading A (0x%02X) into B (0x%02X)\n", a, b);
    #endif
    b = a;
    ip++;
}

static void ld_a_b(void)
{
    #ifdef DUMP
    printf("LD A, B: Loading B (0x%02X) into A (0x%02X)\n", b, a);
    #endif
    a = b;
    ip++;
}

static void sub_a_b(void)
{
    uint32_t eflags;

    #ifdef DUMP
    printf("SUB A, B: Subtracting B (0x%02X) from A (0x%02X)\n", b, a);
    #endif
    __asm__ __volatile__ ("sub al,dl; pushfd; pop edx" : "=a"(a), "=d"(eflags) : "a"(a), "b"(b));

    f = FLAG_SUB;
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;

    ip++;
}

static void xor_a_a(void)
{
    #ifdef DUMP
    printf("XOR A, A: A gets zero.\n");
    #endif
    ip++;
    a = 0;
    f = FLAG_ZERO;
}

static void or_a_c(void)
{
    #ifdef DUMP
    printf("OR A, C: 0x%02X | 0x%02X == 0x%02X\n", a, c, a | c);
    #endif
    a |= c;
    if (!a)
        f = FLAG_ZERO;
    else
        f = 0;
    ip++;
}

static void jp(void)
{
    #ifdef DUMP
    printf("Doing absolute jump from 0x%04X to 0x%04X\n", ip, *((uint16_t *)&memory[ip + 1]));
    #endif
    ip++;
    ip = *((uint16_t *)&memory[ip]);
}

static void ret(void)
{
    ip = pop();
    #ifdef DUMP
    printf("Returning to 0x%04X\n", ip);
    #endif
}

static void prefix0xCB(void)
{
    ip++;

    if (handle0xCB[(int)memory[ip]] == NULL)
    {
        printf("Unknown opcode 0x%02X, prefixed by 0xCB\n", memory[ip]);
        exit(1);
    }

    handle0xCB[(int)memory[ip]]();
}

static void call(void)
{
    ip++;
    #ifdef DUMP
    printf("Calling 0x%04X, pushing 0x%04X\n", *((uint16_t *)&memory[ip]), ip + 2);
    #endif
    push(ip + 2);
    ip = *((uint16_t *)&memory[ip]);
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
        ip++;
    }
}

static void ld__ffn__a(void)
{
    ip++;
    #ifdef DUMP
    printf("LD ($FF00 + n), A: Loading A (0x%02X) into 0x%04X\n", a, 0xFF00 + memory[ip]);
    #endif
    mem_writeb(0xFF00 + memory[ip++], a);
}

static void ld__nn__a(void)
{
    ip++;
    #ifdef DUMP
    printf("LD (nn), A: Loading A (0x%02X) into 0x%04X\n", a, *((uint16_t *)&memory[ip]));
    #endif
    mem_writeb(*((uint16_t *)&memory[ip]), a);
    ip += 2;
}

static void ld_a__ffn(void)
{
    ip++;
    #ifdef DUMP
    printf("LD A, ($FF00 + n): Loading 0x%04X (0x%02X) into A\n", 0xFF00 + memory[ip], memory[0xFF00 + memory[ip]]);
    #endif
    a = memory[0xFF00 + memory[ip++]];
}

static void di_(void)
{
    #ifdef DUMP
    printf("DI called (now %s)\n", ints_enabled ? "enabled" : "disabled");
    #endif
    ip++;
    want_ints_to_be = 0;
}

static void ei_(void)
{
    #ifdef DUMP
    printf("EI called (now %s)\n", ints_enabled ? "enabled" : "disabled");
    #endif
    ip++;
    want_ints_to_be = 1;
}

static void cp_s(void)
{
    uint32_t eflags;

    ip++;
    f = FLAG_SUB;
    __asm__ __volatile__ ("cmp al,bl; pushfd; pop eax" : "=a"(eflags) : "a"(a), "b"(memory[ip]));
    if (eflags & X86_ZF)
        f |= FLAG_ZERO;
    if (eflags & X86_CF)
        f |= FLAG_CRY;
    if (eflags & X86_AF)
        f |= FLAG_HCRY;
    #ifdef DUMP
    printf("CP A, n: Compared A (0x%02X) and 0x%02X, result is 0x%02X\n", a, memory[ip], f);
    if (f & FLAG_ZERO)
        printf(" - A == n\n");
    if (f & FLAG_CRY)
        printf(" - A < n\n");
    #endif
    ip++;
}

static void rst0x38(void)
{
    #ifdef DUMP
    printf("RST $38: Pushing 0x%02X on stack, going to 0x0038\n", ip);
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
    ip++;
    #ifdef DUMP
    printf("RES %i, A: Resetting bit of A (0x%02X to 0x%02X)\n", memory[ip], a, a & ~(1 << memory[ip]));
    #endif
    a &= ~(1 << memory[ip++]);
}

void run(void)
{
    uint64_t last_tsc;

    init_video();

    handle[0x00] = &nop;
    handle[0x01] = &ld_bc_nn;
    handle[0x02] = &ld__bc__a;
    handle[0x03] = &inc_bc;
    handle[0x04] = &inc_b;
    handle[0x05] = &dec_b;
    handle[0x06] = &ld_b_n;
    handle[0x07] = &rlca;
    handle[0x08] = &ld__nn__sp;
    handle[0x09] = &add_hl_bc;
    handle[0x0A] = &ld_a__bc;
    handle[0x0B] = &dec_bc;
    handle[0x0C] = &inc_c;
    handle[0x0D] = &dec_c;
    handle[0x0E] = &ld_c_n;
    handle[0x0F] = &rrca;
    handle[0x10] = &prefix0x10;
    handle[0x18] = &jr;
    handle[0x20] = &jrnz;
    handle[0x21] = &ld_hl_nn;
    handle[0x23] = &inc_hl;
    handle[0x28] = &jrz;
    handle[0x31] = &ld_sp_nn;
    handle[0x36] = &ld__hl__n;
    handle[0x3E] = &ld_a_s;
    handle[0x47] = &ld_b_a;
    handle[0x78] = &ld_a_b;
    handle[0x90] = &sub_a_b;
    handle[0xAF] = &xor_a_a;
    handle[0xB1] = &or_a_c;
    handle[0xC3] = &jp;
    handle[0xC9] = &ret;
    handle[0xCB] = &prefix0xCB;
    handle[0xCD] = &call;
    handle[0xD0] = &retnc;
    handle[0xE0] = &ld__ffn__a;
    handle[0xEA] = &ld__nn__a;
    handle[0xF0] = &ld_a__ffn;
    handle[0xF3] = &di_;
    handle[0xFB] = &ei_;
    handle[0xFE] = &cp_s;
    handle[0xFF] = &rst0x38;

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
            printf("Unknown opcode 0x%02X\n", memory[ip]);
            break;
        }

        handle[(int)memory[ip]]();

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
