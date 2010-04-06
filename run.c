#include <stddef.h>

#include "gbc.h"
#include "gen-operations.h"

// #define DUMP_REGS

#define X86_ZF (1 << 6)
#define X86_CF (1 << 0)
#define X86_AF (1 << 4)

#define nn mem_readw(r_ip)

#ifdef TRUE_TIMING
static uint64_t new_tsc, last_tsc;
static uint32_t diff;
#endif

extern void (*const handle0xCB[64])(void);
extern const int cycles[256];
extern const int cycles0xCB[256];
extern const uint16_t daa_table[1024];

static int add_cycles = 0;



static void nop(void)
{
}

static void prefix0x10(void)
{
    switch (mem_readb(r_ip))
    {
        case 0x00:
            os_print("STOP\n");
            if (!(io_regs->key1 & 1))
                exit_ok();
            else
            {
                double_speed ^= 1;
                os_print("Using %s speed\n", double_speed ? "double" : "single");
            }
        default:
            os_eprint("Unknown opcode 0x%02X, prefixed by 0x10.\n", (unsigned)mem_readb(r_ip));
            exit_err();
    }
}

static void halt(void)
{
    interrupt_issued = 0;
    while (!interrupt_issued)
    {
        update_timer(1);
        generate_interrupts();
    }
}

static void di_(void)
{
    want_ints_to_be = 0;
}

static void ei_(void)
{
    want_ints_to_be = 1;
}

static void jr(void)
{
    r_ip += (int)(signed char)mem_readb(r_ip) + 1;
}

static void jp(void)
{
    r_ip = nn;
}

static void jp__hl(void)
{
    r_ip = r_hl;
}

static void call(void)
{
    push(r_ip + 2);
    r_ip = nn;
}

static void ret(void)
{
    r_ip = pop();
}

static void reti(void)
{
    want_ints_to_be = 1;
    ints_enabled = 1;
    r_ip = pop();
}

static void ld__nn_sp(void)
{
    mem_writew(nn, r_sp);
    r_ip += 2;
}

static void ld__hl_n(void)
{
    mem_writeb(r_hl, mem_readb(r_ip++));
}

static void ldi__hl_a(void)
{
    mem_writeb(r_hl++, r_a);
}

static void ldi_a__hl(void)
{
    r_a = mem_readb(r_hl++);
}

static void ldd__hl_a(void)
{
    mem_writeb(r_hl--, r_a);
}

static void ldd_a__hl(void)
{
    r_a = mem_readb(r_hl--);
}

static void ld__nn_a(void)
{
    mem_writeb(nn, r_a);
    r_ip += 2;
}

static void ld_a__nn(void)
{
    r_a = mem_readb(nn);
    r_ip += 2;
}

static void ld__ffn_a(void)
{
    mem_writeb(0xFF00 + (unsigned)mem_readb(r_ip++), r_a);
}

static void ld_a__ffn(void)
{
    r_a = mem_readb(0xFF00 + (unsigned)mem_readb(r_ip++));
}

static void ld__ffc_a(void)
{
    mem_writeb(0xFF00 + (unsigned)r_c, r_a);
}

static void ld_a__ffc(void)
{
    r_a = mem_readb(0xFF00 + (unsigned)r_c);
}

static void ld_sp_hl(void)
{
    r_sp = r_hl;
}

#ifdef USE_EXT_ASM
extern void ld_hl_spn(void);
#else
static void ld_hl_spn(void)
{
    int val = (int)(signed char)mem_readb(r_ip++);
    unsigned result = (unsigned)(r_sp + val);

    r_f = ((result & 0x10000) >> (16 - FS_CRY)) | (((r_sp ^ (unsigned)val ^ result) & 0x1000) >> (12 - FS_HCRY));
    r_hl = result;
}
#endif

#ifdef USE_EXT_ASM
extern void add_sp_s(void);
#else
static void add_sp_s(void)
{
    int val = (int)(signed char)mem_readb(r_ip++);
    unsigned result = (unsigned)(r_sp + val);

    r_f = ((result & 0x10000) >> (16 - FS_CRY)) | (((r_sp ^ (unsigned)val ^ result) & 0x1000) >> (12 - FS_HCRY));
    r_sp = result;
}
#endif

static void add_hl_hl(void)
{
    r_f = (r_f & FLAG_ZERO) | ((r_hl & 0x8000) >> (15 - FS_CRY)) | ((r_hl & 0x800) >> (11 - FS_CRY));
    r_hl <<= 1;
}

#ifdef USE_EXT_ASM
extern void rlca(void);
#else
static void rlca(void)
{
    r_f = (r_a & 0x80U) >> (7 - FS_CRY);
    __asm__ __volatile__ ("rol $1,%%al" : "=a"(r_a) : "a"(r_a));
}
#endif

#ifdef USE_EXT_ASM
extern void rla(void);
#else
static void rla(void)
{
    unsigned cry = r_f & FLAG_CRY;
    r_f = (r_a & 0x80U) >> (7 - FS_CRY);
    r_a = (r_a << 1) | (cry >> FS_CRY);
}
#endif

#ifdef USE_EXT_ASM
extern void rrca(void);
#else
static void rrca(void)
{
    r_f = (r_a & 0x01) << FS_CRY;
    __asm__ __volatile__ ("ror $1,%%al" : "=a"(r_a) : "a"(r_a));
}
#endif

#ifdef USE_EXT_ASM
extern void rra(void);
#else
static void rra(void)
{
    unsigned cry = r_f & FLAG_CRY;
    r_f = (r_a & 0x01) << FS_CRY;
    r_a = (r_a >> 1) | (cry << (7 - FS_CRY));
}
#endif

static void cpl_a(void)
{
    r_a ^= 0xFF;
    r_f = FLAG_SUB | FLAG_HCRY;
}

static void daa(void)
{
    r_af = daa_table[r_a | ((r_f & 0x70) << 4)];
}

static void scf(void)
{
    r_f = (r_f & FLAG_ZERO) | FLAG_CRY;
}

static void ccf(void)
{
    r_f = (r_f & FLAG_ZERO) | (r_f ^ FLAG_CRY);
}


/// LD ??, nn

LD_xx_nn(bc)
LD_xx_nn(de)
LD_xx_nn(hl)
LD_xx_nn(sp)


/// LD (??), A

LD__xx_A(bc)
LD__xx_A(de)
LD__xx_A(hl)


/// INC ??

INC16(bc)
INC16(de)
INC16(hl)
INC16(sp)


/// DEC ??

DEC16(bc)
DEC16(de)
DEC16(hl)
DEC16(sp)


/// INC ?

#ifdef USE_EXT_ASM

extern void inc_a(void);
extern void inc_b(void);
extern void inc_c(void);
extern void inc_d(void);
extern void inc_e(void);
extern void inc_h(void);
extern void inc_l(void);

#else

INC(a)
INC(b)
INC(c)
INC(d)
INC(e)
INC(h)
INC(l)

#endif

static void inc__hl(void)
{
    uint8_t val = mem_readb(r_hl);
    r_f = (r_f & FLAG_CRY) | (!++val << FS_ZERO);
    r_f |= !(val & 0xF) << FS_HCRY;
    mem_writeb(r_hl, val);
}


/// DEC ?

#ifdef USE_EXT_ASM

extern void dec_a(void);
extern void dec_b(void);
extern void dec_c(void);
extern void dec_d(void);
extern void dec_e(void);
extern void dec_h(void);
extern void dec_l(void);

#else

DEC(a)
DEC(b)
DEC(c)
DEC(d)
DEC(e)
DEC(h)
DEC(l)

#endif

static void dec__hl(void)
{
    uint8_t val = mem_readb(r_hl);
    r_f = FLAG_SUB | (r_f & FLAG_CRY) | (!(val & 0xF) << FS_HCRY);
    r_f |= !--val << FS_ZERO;
    mem_writeb(r_hl, val);
}


/// LD ?, n

LD_x_n(a)
LD_x_n(b)
LD_x_n(c)
LD_x_n(d)
LD_x_n(e)
LD_x_n(h)
LD_x_n(l)


/// ADD HL, ??

#ifdef USE_EXT_ASM

extern void add_hl_bc(void);
extern void add_hl_de(void);
extern void add_hl_sp(void);

#else

ADD_HL(bc)
ADD_HL(de)
ADD_HL(sp)

#endif


/// LD A, (??)

LD_A__(bc)
LD_A__(de)
LD_A__(hl)


/// JRcc n

JRcc( z,   r_f & FLAG_ZERO )
JRcc(nz, !(r_f & FLAG_ZERO))
JRcc( c,   r_f & FLAG_CRY  )
JRcc(nc, !(r_f & FLAG_CRY ))


/// JPcc nn

JPcc( z,   r_f & FLAG_ZERO )
JPcc(nz, !(r_f & FLAG_ZERO))
JPcc( c,   r_f & FLAG_CRY  )
JPcc(nc, !(r_f & FLAG_CRY ))


/// CALLcc nn

CALLcc( z,   r_f & FLAG_ZERO )
CALLcc(nz, !(r_f & FLAG_ZERO))
CALLcc( c,   r_f & FLAG_CRY  )
CALLcc(nc, !(r_f & FLAG_CRY ))


/// RETcc

RETcc( z,   r_f & FLAG_ZERO )
RETcc(nz, !(r_f & FLAG_ZERO))
RETcc( c,   r_f & FLAG_CRY  )
RETcc(nc, !(r_f & FLAG_CRY ))


/// LD ?, ?

static void ld_a_a(void) {}
LD_x_x(a, b)
LD_x_x(a, c)
LD_x_x(a, d)
LD_x_x(a, e)
LD_x_x(a, h)
LD_x_x(a, l)
// "LD A, (HL)" has been defined as an "LD A, (??)" operation

LD_x_x(b, a)
static void ld_b_b(void) {}
LD_x_x(b, c)
LD_x_x(b, d)
LD_x_x(b, e)
LD_x_x(b, h)
LD_x_x(b, l)
LD_x_x_(b, mem_readb(r_hl), _hl)

LD_x_x(c, a)
LD_x_x(c, b)
static void ld_c_c(void) {}
LD_x_x(c, d)
LD_x_x(c, e)
LD_x_x(c, h)
LD_x_x(c, l)
LD_x_x_(c, mem_readb(r_hl), _hl)

LD_x_x(d, a)
LD_x_x(d, b)
LD_x_x(d, c)
static void ld_d_d(void) {}
LD_x_x(d, e)
LD_x_x(d, h)
LD_x_x(d, l)
LD_x_x_(d, mem_readb(r_hl), _hl)

LD_x_x(e, a)
LD_x_x(e, b)
LD_x_x(e, c)
LD_x_x(e, d)
static void ld_e_e(void) {}
LD_x_x(e, h)
LD_x_x(e, l)
LD_x_x_(e, mem_readb(r_hl), _hl)

LD_x_x(h, a)
LD_x_x(h, b)
LD_x_x(h, c)
LD_x_x(h, d)
LD_x_x(h, e)
static void ld_h_h(void) {}
LD_x_x(h, l)
LD_x_x_(h, mem_readb(r_hl), _hl)

LD_x_x(l, a)
LD_x_x(l, b)
LD_x_x(l, c)
LD_x_x(l, d)
LD_x_x(l, e)
LD_x_x(l, h)
static void ld_l_l(void) {}
LD_x_x_(l, mem_readb(r_hl), _hl)


/// LD (HL), ?

// "LD (HL), A" has been defined as an "LD (??), A" operation
LD__HL_x(b)
LD__HL_x(c)
LD__HL_x(d)
LD__HL_x(e)
LD__HL_x(h)
LD__HL_x(l)


/// ADD A, ?

#ifdef USE_EXT_ASM

extern void add_a_b(void);
extern void add_a_c(void);
extern void add_a_d(void);
extern void add_a_e(void);
extern void add_a_h(void);
extern void add_a_l(void);
extern void add_a__hl(void);
extern void add_a_s(void);

#else

ADD_A(b)
ADD_A(c)
ADD_A(d)
ADD_A(e)
ADD_A(h)
ADD_A(l)
ADD_A_(mem_readb(r_hl), _hl)
ADD_A_(mem_readb(r_ip++), s)

#endif

static void add_a_a(void)
{
    if (!r_a)
        r_f = FLAG_ZERO;
    else
    {
        if (r_a & 0x80)
            r_f = FLAG_CRY;
        else
            r_f = 0;
        if (r_a & 0x08)
            r_f |= FLAG_HCRY;
        else
            r_f |= FLAG_HCRY;

        r_a <<= 1;
    }
}


/// ADC A, ?

#ifdef USE_EXT_ASM

extern void adc_a_a(void);
extern void adc_a_b(void);
extern void adc_a_c(void);
extern void adc_a_d(void);
extern void adc_a_e(void);
extern void adc_a_h(void);
extern void adc_a_l(void);
extern void adc_a__hl(void);
extern void adc_a_s(void);

#else

ADC_A(a)
ADC_A(b)
ADC_A(c)
ADC_A(d)
ADC_A(e)
ADC_A(h)
ADC_A(l)
ADC_A_(mem_readb(r_hl), _hl)
ADC_A_(mem_readb(r_ip++), s)

#endif


/// SUB A, ?

#ifdef USE_EXT_ASM

extern void sub_a_b(void);
extern void sub_a_c(void);
extern void sub_a_d(void);
extern void sub_a_e(void);
extern void sub_a_h(void);
extern void sub_a_l(void);
extern void sub_a__hl(void);
extern void sub_a_s(void);

#else

SUB_A(b)
SUB_A(c)
SUB_A(d)
SUB_A(e)
SUB_A(h)
SUB_A(l)
SUB_A_(mem_readb(r_hl), _hl)
SUB_A_(mem_readb(r_ip++), s)

#endif

static void sub_a_a(void)
{
    r_a = 0;
    r_f = FLAG_SUB | FLAG_ZERO;
}


/// SBC A, ?

#ifdef USE_EXT_ASM

extern void sbc_a_b(void);
extern void sbc_a_c(void);
extern void sbc_a_d(void);
extern void sbc_a_e(void);
extern void sbc_a_h(void);
extern void sbc_a_l(void);
extern void sbc_a__hl(void);
extern void sbc_a_s(void);

#else

SBC_A(b)
SBC_A(c)
SBC_A(d)
SBC_A(e)
SBC_A(h)
SBC_A(l)
SBC_A_(mem_readb(r_hl), _hl)
SBC_A_(mem_readb(r_ip++), s)

#endif

static void sbc_a_a(void)
{
    if (r_f & FLAG_CRY)
    {
        r_a = 0xFF;
        r_f = FLAG_SUB | FLAG_ZERO | FLAG_CRY | FLAG_HCRY;
    }
    else
    {
        r_a = 0;
        r_f = FLAG_SUB | FLAG_ZERO;
    }
}


/// AND A, ?

AND_A(b)
AND_A(c)
AND_A(d)
AND_A(e)
AND_A(h)
AND_A(l)
AND_A_(mem_readb(r_hl), _hl)
AND_A_(mem_readb(r_ip++), s)

static void and_a_a(void)
{
    r_f = FLAG_HCRY | (!r_a << FS_ZERO);
}


/// XOR A, ?

XOR_A(b)
XOR_A(c)
XOR_A(d)
XOR_A(e)
XOR_A(h)
XOR_A(l)
XOR_A_(mem_readb(r_hl), _hl)
XOR_A_(mem_readb(r_ip++), s)

static void xor_a_a(void)
{
    r_a = 0;
    r_f = FLAG_ZERO;
}


/// OR A, ?

OR_A(b)
OR_A(c)
OR_A(d)
OR_A(e)
OR_A(h)
OR_A(l)
OR_A_(mem_readb(r_hl), _hl)
OR_A_(mem_readb(r_ip++), s)

static void or_a_a(void)
{
    r_f = !r_a << FS_ZERO;
}


/// CP A, ?

#ifdef USE_EXT_ASM

extern void cp_a_b(void);
extern void cp_a_c(void);
extern void cp_a_d(void);
extern void cp_a_e(void);
extern void cp_a_h(void);
extern void cp_a_l(void);
extern void cp_a__hl(void);
extern void cp_a_s(void);

#else

CP_A(b)
CP_A(c)
CP_A(d)
CP_A(e)
CP_A(h)
CP_A(l)
CP_A_(mem_readb(r_hl), _hl)
CP_A_(mem_readb(r_ip++), s)

#endif

static void cp_a_a(void)
{
    r_f = FLAG_SUB | FLAG_ZERO;
}


/// PUSH ??

PUSH(af)
PUSH(bc)
PUSH(de)
PUSH(hl)


/// POP ??

POP(af)
POP(bc)
POP(de)
POP(hl)


/// RST

RST(0x00)
RST(0x08)
RST(0x10)
RST(0x18)
RST(0x20)
RST(0x28)
RST(0x30)
RST(0x38)


#define BIT_CHECK(regnum, regname) \
    case regnum: \
        r_f = (r_f & FLAG_CRY) | FLAG_HCRY | ((r_##regname & bval) ? 0 : FLAG_ZERO); \
        break;

#define BIT_RESET(regnum, regname) \
    case regnum: \
        r_##regname &= ~bval; \
        break;

#define BIT_SET(regnum, regname) \
    case regnum: \
        r_##regname |= bval; \
        break;

static void prefix0xCB(void)
{
    int bval = 1 << ((mem_readb(r_ip) & 0x38) >> 3);
    int reg = mem_readb(r_ip) & 0x07;

    switch ((mem_readb(r_ip++) & 0xC0) >> 6)
    {
        case 0x01: // BIT
            switch (reg)
            {
                BIT_CHECK(7, a)
                BIT_CHECK(0, b)
                BIT_CHECK(1, c)
                BIT_CHECK(2, d)
                BIT_CHECK(3, e)
                BIT_CHECK(4, h)
                BIT_CHECK(5, l)
                case 6:
                    r_f = (r_f & FLAG_CRY) | FLAG_HCRY | ((mem_readb(r_hl) & bval) ? 0 : FLAG_ZERO); \
                    break;
            }
            break;
        case 0x02: // RES
            switch (reg)
            {
                BIT_RESET(7, a)
                BIT_RESET(0, b)
                BIT_RESET(1, c)
                BIT_RESET(2, d)
                BIT_RESET(3, e)
                BIT_RESET(4, h)
                BIT_RESET(5, l)
                case 6:
                    mem_writeb(r_hl, mem_readb(r_hl) & ~bval);
            }
            break;
        case 0x03: // SET
            switch (reg)
            {
                BIT_SET(7, a)
                BIT_SET(0, b)
                BIT_SET(1, c)
                BIT_SET(2, d)
                BIT_SET(3, e)
                BIT_SET(4, h)
                BIT_SET(5, l)
                case 6:
                    mem_writeb(r_hl, mem_readb(r_hl) | bval);
            }
            break;
        default:
            if (handle0xCB[(int)mem_readb(r_ip - 1)] == NULL)
            {
                os_print("Unknown opcode 0x%02X, prefixed by 0xCB\n", (unsigned)mem_readb(r_ip - 1));
                exit_err();
            }

            handle0xCB[(int)mem_readb(r_ip - 1)]();
    }
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
    &ld_a_n,
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
    &rst0x10,
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
    &cp_a_s,
    &rst0x38        // 0xFF
};

void run(void)
{
    #ifdef TRUE_TIMING
    int64_t too_short = 0, collect_sleep_time = 0;

    tsc_resolution = determine_tsc_resolution();
    #endif

    init_video();

    r_ip = 0x0100;
    r_sp = 0xFFFE;
    r_af = 0x11B0;
    r_bc = 0x0000;
    r_de = 0xFF56;
    r_hl = 0x000D;

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
    wtm[0] = (uint8_t *)&vidram[0x1800];
    wtm[1] = (uint8_t *)&vidram[0x3800];

    for(int i = 0; i < 32; i++)
    {
        bpalette[i] = 0x7FFF;
        opalette[i] = 0x7FFF;
    }

    #ifdef TRUE_TIMING
    last_tsc = rdtsc();
    #endif

    for (;;)
    {
        int change_int_status = 0, cyc, opcode;

        if (want_ints_to_be != ints_enabled)
            change_int_status = 1;

        opcode = mem_readb(r_ip);
        if (handle[opcode] == NULL)
        {
            os_print("Unknown opcode 0x%02X\n", opcode);
            exit_err();
        }

        #ifdef DUMP_REGS
        os_print("PC=%04x AF=%04x BC=%04x DE=%04x HL=%04x SP=%04x\n", (unsigned)r_ip, (unsigned)r_af, (unsigned)r_bc, (unsigned)r_de, (unsigned)r_hl, (unsigned)r_sp);
        #endif

        r_ip++;

        if (opcode != 0xCB)
            cyc = cycles[opcode];
        else
            cyc = cycles0xCB[(int)mem_readb(r_ip)];

        add_cycles = 0;

        handle[opcode]();

        cyc += add_cycles;

        if (change_int_status)
        {
            #ifdef DUMP_REGS
            os_print("Changing interrupt status from %s to %s\n", ints_enabled ? "enabled" : "disabled", want_ints_to_be ? "enabled" : "disabled");
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

        if (!boost)
        {
            too_short = cyc - 1000LL * (uint64_t)diff / (uint64_t)tsc_resolution;
            collect_sleep_time += too_short;

            if (collect_sleep_time >= 10000)
            {
                sleep_ms(collect_sleep_time);
                collect_sleep_time = 0;
            }
        }
        #endif
    }
}
