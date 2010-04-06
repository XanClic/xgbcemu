#ifndef GEN_OPERATIONS_H
#define GEN_OPERATIONS_H

/// LD ?, n

#define LD_x_n(reg) \
static void ld_##reg##_n(void) \
{ \
    r_##reg = mem_readb(r_ip++); \
}


/// LD ??, nn

#define LD_xx_nn(reg) \
static void ld_##reg##_nn(void) \
{ \
    r_##reg = nn; \
    r_ip += 2; \
}


/// LD (??), A

#define LD__xx_A(reg) \
static void ld__##reg##_a(void) \
{ \
    mem_writeb(r_##reg, r_a); \
}


/// LD A, (??)

#define LD_A__(reg) \
static void ld_a__##reg(void) \
{ \
    r_a = mem_readb(r_##reg); \
}


/// LD ?, ?

#define LD_x_x_(r1, r2, r2a) \
static void ld_##r1##_##r2a(void) \
{ \
    r_##r1 = r2; \
}

#define LD_x_x(r1, r2) LD_x_x_(r1, r_##r2, r2)


/// LD (HL), ?

#define LD__HL_x(reg) \
static void ld__hl_##reg(void) \
{ \
    mem_writeb(r_hl, r_##reg); \
}


/// INC ?

#define INC(reg) \
static void inc_##reg(void) \
{ \
    r_f = (r_f & FLAG_CRY) | (!++r_##reg << FS_ZERO); \
    r_f |= !(r_##reg & 0xF) << FS_HCRY; \
}


/// INC ??

#define INC16(reg) \
static void inc_##reg(void) \
{ \
    r_##reg++; \
}


/// DEC ?

#define DEC(reg) \
static void dec_##reg(void) \
{ \
    r_f = FLAG_SUB | (r_f & FLAG_CRY) | (!(r_##reg & 0xF) << FS_HCRY); \
    r_f |= !--r_##reg << FS_ZERO; \
}


/// DEC ??

#define DEC16(reg) \
static void dec_##reg(void) \
{ \
    r_##reg--; \
}


/// ADD HL, ??

#define ADD_HL(reg) \
static void add_hl_##reg(void) \
{ \
    unsigned result = r_hl + r_##reg; \
    r_f = (r_f & FLAG_ZERO) | ((result & 0x10000) >> (16 - FS_CRY)) | (((r_hl ^ r_bc ^ result) & 0x1000U) >> (12 - FS_HCRY)); \
    r_hl = (uint16_t)result; \
}


/// ADD A, ?

#define ADD_A_(reg, ar) \
static void add_a_##ar(void) \
{ \
    uint8_t rval = reg; \
    uint16_t result = (uint16_t)(r_a + rval); \
    r_f = ((result & 0x100U) >> (8 - FS_CRY)) | (!result << FS_ZERO); \
    if ((r_a & 0xF) + (rval & 0xF) > 0xF) \
        r_f |= FLAG_HCRY; \
    r_a = (uint8_t)result; \
}

#define ADD_A(reg) ADD_A_(r_##reg, reg)


/// ADC A, ?

#define ADC_A_(reg, ar) \
static void adc_a_##ar(void) \
{ \
    uint8_t rval = reg; \
    int cry = (r_f & FLAG_CRY) >> FS_CRY; \
    uint16_t result = (uint16_t)(r_a + rval + cry); \
    r_f = ((result & 0x100U) >> (8 - FS_CRY)) | (!result << FS_ZERO); \
    if ((r_a & 0xF) + (rval & 0xF) + cry > 0xF) \
        r_f |= FLAG_HCRY; \
    r_a = (uint8_t)result; \
}

#define ADC_A(reg) ADC_A_(r_##reg, reg)


/// SUB A, ?

#define SUB_A_(reg, ar) \
static void sub_a_##ar(void) \
{ \
    uint8_t rval = reg; \
    uint16_t result = (uint16_t)(r_a - rval); \
    r_f = FLAG_SUB | ((result & 0x100U) >> (8 - FS_CRY)) | (!result << FS_ZERO); \
    if ((r_a & 0xFU) - (rval & 0xFU) > 0xFU) \
        r_f |= FLAG_HCRY; \
    r_a = (uint8_t)result; \
}

#define SUB_A(reg) SUB_A_(r_##reg, reg)


/// SBC A, ?

#define SBC_A_(reg, ar) \
static void sbc_a_##ar(void) \
{ \
    uint8_t rval = reg; \
    int cry = (r_f & FLAG_CRY) >> FS_CRY; \
    uint16_t result = (uint16_t)(r_a - rval - cry); \
    r_f = FLAG_SUB | ((result & 0x100U) >> (8 - FS_CRY)) | (!result << FS_ZERO); \
    if ((r_a & 0xFU) - (rval & 0xFU) - (unsigned)cry > 0xFU) \
        r_f |= FLAG_HCRY; \
    r_a = (uint8_t)result; \
}

#define SBC_A(reg) SBC_A_(r_##reg, reg)


/// CP A, ?

#define CP_A_(reg, ar) \
static void cp_a_##ar(void) \
{ \
    uint8_t rval = reg; \
    uint16_t result = (uint16_t)(r_a - rval); \
    r_f = FLAG_SUB | ((result & 0x100U) >> (8 - FS_CRY)) | (!result << FS_ZERO); \
    if ((r_a & 0xFU) - (rval & 0xFU) > 0xFU) \
        r_f |= FLAG_HCRY; \
}

#define CP_A(reg) CP_A_(r_##reg, reg)


/// AND A, ?

#define AND_A_(reg, ar) \
static void and_a_##ar(void) \
{ \
    r_a &= reg; \
    r_f = FLAG_HCRY | (!r_a << FS_ZERO); \
}

#define AND_A(reg) AND_A_(r_##reg, reg)


/// XOR A, ?

#define XOR_A_(reg, ar) \
static void xor_a_##ar(void) \
{ \
    r_a ^= reg; \
    r_f = !r_a << FS_ZERO; \
}

#define XOR_A(reg) XOR_A_(r_##reg, reg)


/// OR A, ?

#define OR_A_(reg, ar) \
static void or_a_##ar(void) \
{ \
    r_a |= reg; \
    r_f = !r_a << FS_ZERO; \
}

#define OR_A(reg) OR_A_(r_##reg, reg)


/// JRcc n

#define JRcc(cc, condition) \
static void jr##cc(void) \
{ \
    if (condition) \
    { \
        jr(); \
        add_cycles = 1; \
    } \
    else \
        r_ip++; \
}


/// JPcc nn

#define JPcc(cc, condition) \
static void jp##cc(void) \
{ \
    if (condition) \
    { \
        jp(); \
        add_cycles = 1; \
    } \
    else \
        r_ip += 2; \
}


/// CALLcc nn

#define CALLcc(cc, condition) \
static void call##cc(void) \
{ \
    if (condition) \
    { \
        call(); \
        add_cycles = 3; \
    } \
    else \
        r_ip += 2; \
}


/// RETcc

#define RETcc(cc, condition) \
static void ret##cc(void) \
{ \
    if (condition) \
    { \
        ret(); \
        add_cycles = 3; \
    } \
}


/// PUSH ??

#define PUSH(reg) \
static void push_##reg(void) \
{ \
    push(r_##reg); \
}


/// POP ??

#define POP(reg) \
static void pop_##reg(void) \
{ \
    r_##reg = pop(); \
}


/// RST

#define RST(addr) \
static void rst##addr(void) \
{ \
    push(r_ip); \
    r_ip = addr; \
}

#endif
