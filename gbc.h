#ifndef GBC_H
#define GBC_H

#include <stdint.h>
#include <stdio.h>

#include "os-advio.h"
#include "os-def.h"
#include "os-io.h"
#include "os-std.h"
#include "os-time.h"


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
    uint8_t rsvd6;
    uint8_t key1;
    uint8_t rsvd7;
    uint8_t vbk;
    uint8_t rsvd8;
    uint8_t hdma1, hdma2, hdma3, hdma4, hdma5;
    uint8_t rsvd9[18];
    uint8_t bcps;
    uint8_t bcpd;
    uint8_t ocps;
    uint8_t ocpd;
    uint8_t rsvd10[4];
    uint8_t svbk;
    uint8_t rsvd11[142];
    uint8_t int_enable;
} __attribute__((packed));


extern struct io *io_regs;
extern int card_type, rom_size, ram_size;
extern uint16_t r_ip, r_sp, r_af, r_bc, r_de, r_hl;
extern int ints_enabled, want_ints_to_be, lcd_on, double_speed, gbc_mode;
extern volatile int interrupt_issued, keystates;
extern uint32_t tsc_resolution;
extern void *vidmem;
extern file_obj fp, save;
extern int mbc, ext_ram, rtc, batt, rmbl, current_rom_bank;
extern uint8_t *ext_ram_ptr, *rom_bank_ptr, *base_rom_ptr;
extern uint8_t *int_ram, *oam_io, *vidram, *int_wram, *full_int_wram, *full_vidram;
extern uint16_t bpalette[32], opalette[32];
extern uint8_t *btm[2], *bwtd[2], *wtm[2];
extern int hdma_on, boost;
extern int doy_diff, hour_diff, min_diff, sec_diff;
#ifdef ENABLE_LINK
extern tcp_server_t server;
extern tcp_connection_t current_connection;
extern volatile int waiting_for_conn_ack;
extern int connection_master;
#endif
#define r_a (((uint8_t *)&r_af)[1])
#define r_b (((uint8_t *)&r_bc)[1])
#define r_c (((uint8_t *)&r_bc)[0])
#define r_d (((uint8_t *)&r_de)[1])
#define r_e (((uint8_t *)&r_de)[0])
#define r_f (((uint8_t *)&r_af)[0])
#define r_h (((uint8_t *)&r_hl)[1])
#define r_l (((uint8_t *)&r_hl)[0])

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

#define FS_ZERO   7
#define FS_SUB    6
#define FS_HCRY   5
#define FS_CRY    4
#define FLAG_ZERO (1U << FS_ZERO)
#define FLAG_SUB  (1U << FS_SUB)
#define FLAG_HCRY (1U << FS_HCRY)
#define FLAG_CRY  (1U << FS_CRY)

#define INT_P10_P13   (1 << 4)
#define INT_SERIAL    (1 << 3)
#define INT_TIMER     (1 << 2)
#define INT_LCDC_STAT (1 << 1)
#define INT_VBLANK    (1 << 0)

#ifdef VK_A
#undef VK_A
#endif
#ifdef VK_B
#undef VK_B
#endif
#ifdef VK_SELECT
#undef VK_SELECT
#endif
#ifdef VK_START
#undef VK_START
#endif
#ifdef VK_RIGHT
#undef VK_RIGHT
#endif
#ifdef VK_LEFT
#undef VK_LEFT
#endif
#ifdef VK_UP
#undef VK_UP
#endif
#ifdef VK_DOWN
#undef VK_DOWN
#endif

#define VK_A      KEY_A
#define VK_B      KEY_B
#define VK_SELECT KEY_SELECT
#define VK_START  KEY_START
#define VK_RIGHT  (KEY_RIGHT << 4)
#define VK_LEFT   (KEY_LEFT  << 4)
#define VK_UP     (KEY_UP    << 4)
#define VK_DOWN   (KEY_DOWN  << 4)

#define set_day_of_year() (current_day_of_year() + doy_diff )
#define set_hour()        (current_hour()        + hour_diff)
#define set_minutes()     (current_minutes()     + min_diff )
#define set_seconds()     (current_seconds()     + sec_diff )

#define LINK_PORT 4224

static inline int pal2rgb(int pal)
{
    return ((pal & 0x1F) << 19) | (((pal >> 5) & 0x1F) << 11) | ((pal >> 10) << 3);
}


void decrease_frameskip(void);
void draw_line(int line);
void enter_shell(void);
void generate_interrupts(void);
void hdma_copy_16b(void);
void increase_frameskip(void);
void init_memory(void);
void init_video(int multiplier);
void io_outb(uint8_t reg, uint8_t val);
#ifdef ENABLE_LINK
void link_clock(void);
void link_connect(const char *dest);
void link_data_arrived(tcp_connection_t conn, void *data, size_t size);
void link_start_ext_transfer(void);
void link_unplug(void);
#endif
void load_memory(void);
void load_rom(const char *fname, const char *sname, int zoom);
void mem_writeb(uintptr_t addr, uint8_t value);
void mem_writew(uintptr_t addr, uint16_t value);
uint8_t mem_readb(uintptr_t addr);
uint16_t mem_readw(uintptr_t addr);
#ifdef ENABLE_LINK
void new_client(tcp_connection_t conn);
#endif
uint16_t pop(void);
void push(uint16_t value);
void redraw(void);
void run(int zoom);
void save_to_disk(void);
void update_keyboard(void);
void update_timer(int cycles_gone);

#endif
