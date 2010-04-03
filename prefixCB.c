#include "gbc.h"

// #define DUMP

#ifdef DUMP

#define RLC(sr, cr) \
    static void rlc_##sr(void) \
    { \
        printf("RLC " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        f = (sr & 0x80) ? FLAG_CRY : 0; \
        __asm__ __volatile__ ("rol al,1" : "=a"(sr) : "a"(sr)); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define RRC(sr, cr) \
    static void rrc_##sr(void) \
    { \
        printf("RRC " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        f = (sr & 0x01) * FLAG_CRY; \
        __asm__ __volatile__ ("ror al,1" : "=a"(sr) : "a"(sr)); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define RL(sr, cr) \
    static void rl_##sr(void) \
    { \
        int cry = f & FLAG_CRY; \
        printf("RL " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        f = (sr & 0x80) ? FLAG_CRY : 0; \
        sr = ((sr << 1) & 0xFF) | !!cry; \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define RR(sr, cr) \
    static void rr_##sr(void) \
    { \
        int cry = f & FLAG_CRY; \
        printf("RL " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        f = (sr & 0x01) * FLAG_CRY; \
        sr = (sr >> 1) | (!!cry << 7); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SLA(sr, cr) \
    static void sla_##sr(void) \
    { \
        printf("SLA " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        f = (sr & 0x80) ? FLAG_CRY : 0; \
        sr <<= 1; \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SRA(sr, cr) \
    static void sra_##sr(void) \
    { \
        printf("SRA " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        f = (sr & 0x01) * FLAG_CRY; \
        sr = (sr >> 1) | (sr & 0x80); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SRL(sr, cr) \
    static void srl_##sr(void) \
    { \
        printf("SRL " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        f = (sr & 0x01) * FLAG_CRY; \
        sr >>= 1; \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SWAP(sr, cr) \
    static void swap_##sr(void) \
    { \
        printf("SWAP " #cr ": " #cr " == 0x%02X\n", (unsigned)sr); \
        if (!sr) \
            f = FLAG_ZERO; \
        else \
        { \
            sr = (sr << 4) | (sr >> 4); \
            f = 0; \
        } \
    }



#else



#define RLC(sr, cr) \
    static void rlc_##sr(void) \
    { \
        f = (sr & 0x80) ? FLAG_CRY : 0; \
        __asm__ __volatile__ ("rol al,1" : "=a"(sr) : "a"(sr)); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define RRC(sr, cr) \
    static void rrc_##sr(void) \
    { \
        f = (sr & 0x01) * FLAG_CRY; \
        __asm__ __volatile__ ("ror al,1" : "=a"(sr) : "a"(sr)); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define RL(sr, cr) \
    static void rl_##sr(void) \
    { \
        int cry = f & FLAG_CRY; \
        f = (sr & 0x80) ? FLAG_CRY : 0; \
        sr = ((sr << 1) & 0xFF) | !!cry; \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define RR(sr, cr) \
    static void rr_##sr(void) \
    { \
        int cry = f & FLAG_CRY; \
        f = (sr & 0x01) * FLAG_CRY; \
        sr = (sr >> 1) | (!!cry << 7); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SLA(sr, cr) \
    static void sla_##sr(void) \
    { \
        f = (sr & 0x80) ? FLAG_CRY : 0; \
        sr <<= 1; \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SRA(sr, cr) \
    static void sra_##sr(void) \
    { \
        f = (sr & 0x01) * FLAG_CRY; \
        sr = (sr >> 1) | (sr & 0x80); \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SRL(sr, cr) \
    static void srl_##sr(void) \
    { \
        f = (sr & 0x01) * FLAG_CRY; \
        sr >>= 1; \
        if (!sr) \
            f |= FLAG_ZERO; \
    }

#define SWAP(sr, cr) \
    static void swap_##sr(void) \
    { \
        if (!sr) \
            f = FLAG_ZERO; \
        else \
        { \
            sr = (sr << 4) | (sr >> 4); \
            f = 0; \
        } \
    }

#endif

RLC(a, A)
RLC(b, B)
RLC(c, C)
RLC(d, D)
RLC(e, E)
RLC(h, H)
RLC(l, L)
RRC(a, A)
RRC(b, B)
RRC(c, C)
RRC(d, D)
RRC(e, E)
RRC(h, H)
RRC(l, L)
RL(a, A)
RL(b, B)
RL(c, C)
RL(d, D)
RL(e, E)
RL(h, H)
RL(l, L)
RR(a, A)
RR(b, B)
RR(c, C)
RR(d, D)
RR(e, E)
RR(h, H)
RR(l, L)
SLA(a, A)
SLA(b, B)
SLA(c, C)
SLA(d, D)
SLA(e, E)
SLA(h, H)
SLA(l, L)
SRA(a, A)
SRA(b, B)
SRA(c, C)
SRA(d, D)
SRA(e, E)
SRA(h, H)
SRA(l, L)
SRL(a, A)
SRL(b, B)
SRL(c, C)
SRL(d, D)
SRL(e, E)
SRL(h, H)
SRL(l, L)
SWAP(a, A)
SWAP(b, B)
SWAP(c, C)
SWAP(d, D)
SWAP(e, E)
SWAP(h, H)
SWAP(l, L)

static void rlc__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("RLC (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (val & 0x80) ? FLAG_CRY : 0;
    __asm__ __volatile__ ("rol al,1" : "=a"(val) : "a"(val));
    if (!val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void rrc__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("RRC (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (val & 0x01) * FLAG_CRY;
    __asm__ __volatile__ ("ror al,1" : "=a"(val) : "a"(val));
    if (!val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void rl__hl(void)
{
    int cry = f & FLAG_CRY;
    uint8_t val;

    #ifdef DUMP
    printf("RL (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (val & 0x80) ? FLAG_CRY : 0;
    val = ((val << 1) & 0xFF) | !!cry;
    if (!val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void rr__hl(void)
{
    int cry = f & FLAG_CRY;
    uint8_t val;

    #ifdef DUMP
    printf("RR (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (val & 0x01) ? FLAG_CRY : 0;
    val = (val >> 1) | (!!cry << 7);
    if (!val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void sla__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("SLA (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (val & 0x80) ? FLAG_CRY : 0;
    val <<= 1;
    if (!val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void sra__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("SRA (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (val & 0x01) * FLAG_CRY;
    val = (val >> 1) | (val & 0x80);
    if (!val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void srl__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("SRL (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    f = (val & 0x01) * FLAG_CRY;
    val >>= 1;
    if (!val)
        f |= FLAG_ZERO;
    mem_writeb(hl, val);
}

static void swap__hl(void)
{
    uint8_t val;

    #ifdef DUMP
    printf("SWAP (HL): HL == 0x%04X\n", (unsigned)hl);
    #endif

    val = mem_readb(hl);
    if (!val)
        f = FLAG_ZERO;
    else
    {
        val = (val << 4) | (val >> 4);
        f = 0;
        mem_writeb(hl, val);
    }
}

void (*const handle0xCB[64])(void) =
{
    &rlc_b,     // 0x00
    &rlc_c,
    &rlc_d,
    &rlc_e,
    &rlc_h,
    &rlc_l,
    &rlc__hl,
    &rlc_a,
    &rrc_b,     // 0x08,
    &rrc_c,
    &rrc_d,
    &rrc_e,
    &rrc_h,
    &rrc_l,
    &rrc__hl,
    &rrc_a,
    &rl_b,      // 0x10
    &rl_c,
    &rl_d,
    &rl_e,
    &rl_h,
    &rl_l,
    &rl__hl,
    &rl_a,
    &rr_b,      // 0x18
    &rr_c,
    &rr_d,
    &rr_e,
    &rr_h,
    &rr_l,
    &rr__hl,
    &rr_a,
    &sla_b,     // 0x20
    &sla_c,
    &sla_d,
    &sla_e,
    &sla_h,
    &sla_l,
    &sla__hl,
    &sla_a,
    &sra_b,     // 0x28
    &sra_c,
    &sra_d,
    &sra_e,
    &sra_h,
    &sra_l,
    &sra__hl,
    &sra_a,
    &swap_b,    // 0x30
    &swap_c,
    &swap_d,
    &swap_e,
    &swap_h,
    &swap_l,
    &swap__hl,
    &swap_a,
    &srl_b,     // 0x38
    &srl_c,
    &srl_d,
    &srl_e,
    &srl_h,
    &srl_l,
    &srl__hl,
    &srl_a      // 0x3F
};
