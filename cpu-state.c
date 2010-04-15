#include <stddef.h>
#include <stdint.h>

#include "os-advio.h"
#include "os-def.h"
#include "os-io.h"

void *io_regs = NULL;
uint16_t r_ip, r_sp, r_af, r_bc, r_de, r_hl;
int rom_size, ram_size, ints_enabled = 1, want_ints_to_be = 1, lcd_on = 1, double_speed = 0, gbc_mode;
volatile int interrupt_issued = 0, keystates = 0;
uint32_t tsc_resolution = 0;
void *vidmem = NULL;
file_obj fp, save;

int mbc, ext_ram, rtc, batt, rmbl, current_rom_bank = 0;
uint8_t *ext_ram_ptr = NULL, *rom_bank_ptr = NULL, *base_rom_ptr = NULL;
uint8_t *int_ram = NULL, *oam_io = NULL, *vidram = NULL, *int_wram = NULL, *full_int_wram = NULL, *full_vidram = NULL;

uint16_t bpalette[32], opalette[32];
uint8_t *btm[2] = { NULL }, *bwtd[2] = { NULL }, *wtm[2] = { NULL };

int hdma_on = 0, boost = 0;
int doy_diff = 0, hour_diff = 0, min_diff = 0, sec_diff = 0;
#ifdef ENABLE_LINK
tcp_server_t server;
tcp_connection_t current_connection = INVALID_CONN_VALUE;
volatile int waiting_for_conn_ack = 0;
int connection_master = 0;
#endif
