#ifndef GBC_H
#define GBC_H

#include <stdint.h>


struct io
{
    uint8_t p1;
    uint8_t sb;
    uint8_t sc;
    uint8_t rsvd1;
    uint8_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
    uint8_t rsvd2[7];
    uint8_t int_flag;
    uint8_t nr10;
    uint8_t nr11;
    uint8_t nr12;
    uint8_t nr13;
    uint8_t nr14;
    uint8_t rsvd3;
    uint8_t nr21;
    uint8_t nr22;
    uint8_t nr23;
    uint8_t nr24;
    uint8_t nr30;
    uint8_t nr31;
    uint8_t nr32;
    uint8_t nr33;
    uint8_t nr34;
    uint8_t rsvd4;
    uint8_t nr41;
    uint8_t nr42;
    uint8_t nr43;
    uint8_t nr44;
    uint8_t nr50;
    uint8_t nr51;
    uint8_t nr52;
    uint8_t rsvd5[9];
    uint8_t wave_pat[16];
    uint8_t lcdc;
    uint8_t stat;
    uint8_t scy;
    uint8_t scx;
    uint8_t ly;
    uint8_t lyc;
    uint8_t dma;
    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;
    uint8_t wy;
    uint8_t wx;
    uint8_t rsvd6[179];
    uint8_t int_enable;
} __attribute__((packed));


extern char *memory;
extern struct io *io_regs;
extern int card_type, rom_size, ram_size;
extern uint16_t _ip, _sp, _af, _bc, _de, _hl;
#define ip _ip
#define sp _sp
#define af _af
#define bc _bc
#define de _de
#define hl _hl
#define a (((uint8_t *)&_af)[1])
#define b (((uint8_t *)&_bc)[1])
#define c (((uint8_t *)&_bc)[0])
#define d (((uint8_t *)&_de)[1])
#define e (((uint8_t *)&_de)[0])
#define f (((uint8_t *)&_af)[0])
#define h (((uint8_t *)&_hl)[1])
#define l (((uint8_t *)&_hl)[0])
extern int ints_enabled;

#define KEY_A      (1 << 0)
#define KEY_B      (1 << 1)
#define KEY_SELECT (1 << 2)
#define KEY_START  (1 << 3)

#define KEY_RIGHT  (1 << 0)
#define KEY_LEFT   (1 << 1)
#define KEY_UP     (1 << 2)
#define KEY_DOWN   (1 << 3)

#define KEY_DIR (1 << 4)
#define KEY_OTH (1 << 5)

#define FLAG_ZERO (1 << 7)
#define FLAG_SUB  (1 << 6)
#define FLAG_HCRY (1 << 5)
#define FLAG_CRY  (1 << 4)


void load_rom(const char *fname);
void run(void);

#endif
