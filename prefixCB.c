#include "gbc.h"

#define RLC(sr, cr) \
    static void rlc_##sr(void) \
    { \
        r_f = (r_##sr & 0x80U) >> (7 - FS_CRY); \
        r_##sr = (r_##sr << 1) | (r_##sr >> 7); \
        if (!r_##sr) \
            r_f |= FLAG_ZERO; \
    }

#define RRC(sr, cr) \
    static void rrc_##sr(void) \
    { \
        r_f = (r_##sr & 0x01) << FS_CRY; \
        r_##sr = (r_##sr >> 1) | (r_##sr << 7); \
        if (!r_##sr) \
            r_f |= FLAG_ZERO; \
    }

#define RL(sr, cr) \
    static void rl_##sr(void) \
    { \
        unsigned cry = r_f & FLAG_CRY; \
        r_f = (r_##sr & 0x80U) >> (7 - FS_CRY); \
        r_##sr = (r_##sr << 1) | (cry >> FS_CRY); \
        if (!r_##sr) \
            r_f |= FLAG_ZERO; \
    }

#define RR(sr, cr) \
    static void rr_##sr(void) \
    { \
        unsigned cry = r_f & FLAG_CRY; \
        r_f = (r_##sr & 0x01) << FS_CRY; \
        r_##sr = (r_##sr >> 1) | (cry << (7 - FS_CRY)); \
        if (!r_##sr) \
            r_f |= FLAG_ZERO; \
    }

#define SLA(sr, cr) \
    static void sla_##sr(void) \
    { \
        r_f = (r_##sr & 0x80U) >> (7 - FS_CRY); \
        r_##sr <<= 1; \
        if (!r_##sr) \
            r_f |= FLAG_ZERO; \
    }

#define SRA(sr, cr) \
    static void sra_##sr(void) \
    { \
        r_f = (r_##sr & 0x01) << FS_CRY; \
        r_##sr = (int8_t)r_##sr >> 1; \
        if (!r_##sr) \
            r_f |= FLAG_ZERO; \
    }

#define SRL(sr, cr) \
    static void srl_##sr(void) \
    { \
        r_f = (r_##sr & 0x01) << FS_CRY; \
        r_##sr >>= 1; \
        if (!r_##sr) \
            r_f |= FLAG_ZERO; \
    }

#define SWAP(sr, cr) \
    static void swap_##sr(void) \
    { \
        if (!r_##sr) \
            r_f = FLAG_ZERO; \
        else \
        { \
            r_##sr = (r_##sr << 4) | (r_##sr >> 4); \
            r_f = 0; \
        } \
    }

#ifdef USE_EXT_ASM
void rlc_a(void);
void rlc_b(void);
void rlc_c(void);
void rlc_d(void);
void rlc_e(void);
void rlc_h(void);
void rlc_l(void);
void rlc__hl(void);
void rrc_a(void);
void rrc_b(void);
void rrc_c(void);
void rrc_d(void);
void rrc_e(void);
void rrc_h(void);
void rrc_l(void);
void rrc__hl(void);
void rl_a(void);
void rl_b(void);
void rl_c(void);
void rl_d(void);
void rl_e(void);
void rl_h(void);
void rl_l(void);
void rl__hl(void);
void rr_a(void);
void rr_b(void);
void rr_c(void);
void rr_d(void);
void rr_e(void);
void rr_h(void);
void rr_l(void);
void rr__hl(void);
void sla_a(void);
void sla_b(void);
void sla_c(void);
void sla_d(void);
void sla_e(void);
void sla_h(void);
void sla_l(void);
void sla__hl(void);
void sra_a(void);
void sra_b(void);
void sra_c(void);
void sra_d(void);
void sra_e(void);
void sra_h(void);
void sra_l(void);
void sra__hl(void);
void srl_a(void);
void srl_b(void);
void srl_c(void);
void srl_d(void);
void srl_e(void);
void srl_h(void);
void srl_l(void);
void srl__hl(void);
void swap_a(void);
void swap_b(void);
void swap_c(void);
void swap_d(void);
void swap_e(void);
void swap_h(void);
void swap_l(void);
void swap__hl(void);
#else
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
    uint8_t val = mem_readb(r_hl);
    r_f = (val & 0x80U) >> (7 - FS_CRY);
    val = (val << 1) | (val >> 7);
    if (!val)
        r_f |= FLAG_ZERO;
    mem_writeb(r_hl, val);
}

static void rrc__hl(void)
{
    uint8_t val = mem_readb(r_hl);
    r_f = (val & 0x01) << FS_CRY;
    val = (val >> 1) | (val << 7);
    if (!val)
        r_f |= FLAG_ZERO;
    mem_writeb(r_hl, val);
}

static void rl__hl(void)
{
    int cry = r_f & FLAG_CRY;
    uint8_t val = mem_readb(r_hl);
    r_f = (val & 0x80U) >> (7 - FS_CRY);
    val = ((val << 1) & 0xFF) | (cry >> FS_CRY);
    if (!val)
        r_f |= FLAG_ZERO;
    mem_writeb(r_hl, val);
}

static void rr__hl(void)
{
    int cry = r_f & FLAG_CRY;
    uint8_t val = mem_readb(r_hl);
    r_f = (val & 0x01) << FS_CRY;
    val = (val >> 1) | (cry << (7 - FS_CRY));
    if (!val)
        r_f |= FLAG_ZERO;
    mem_writeb(r_hl, val);
}

static void sla__hl(void)
{
    uint8_t val = mem_readb(r_hl);
    r_f = (val & 0x80U) >> (7 - FS_CRY);
    val <<= 1;
    if (!val)
        r_f |= FLAG_ZERO;
    mem_writeb(r_hl, val);
}

static void sra__hl(void)
{
    uint8_t val = mem_readb(r_hl);
    r_f = (val & 0x01) << FS_CRY;
    val = (int8_t)val >> 1;
    if (!val)
        r_f |= FLAG_ZERO;
    mem_writeb(r_hl, val);
}

static void srl__hl(void)
{
    uint8_t val = mem_readb(r_hl);
    r_f = (val & 0x01) << FS_CRY;
    val >>= 1;
    if (!val)
        r_f |= FLAG_ZERO;
    mem_writeb(r_hl, val);
}

static void swap__hl(void)
{
    uint8_t val = mem_readb(r_hl);
    if (!val)
        r_f = FLAG_ZERO;
    else
    {
        val = (val << 4) | (val >> 4);
        r_f = 0;
        mem_writeb(r_hl, val);
    }
}
#endif

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
