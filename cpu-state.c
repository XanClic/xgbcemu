#include <stddef.h>
#include <stdint.h>

#include "os-io.h"

void *io_regs = NULL;
uint16_t r_ip, r_sp, r_af, r_bc, r_de, r_hl;
int rom_size, ram_size, ints_enabled = 1, want_ints_to_be = 1, lcd_on = 1, double_speed = 0;
volatile int interrupt_issued = 0, keystates = 0;
uint32_t tsc_resolution = 0;
void *vidmem = NULL;
file_obj fp, save;

int mbc, ext_ram, rtc, batt, rmbl;
uint8_t *ext_ram_ptr = NULL, *rom_bank_ptr = NULL, *base_rom_ptr = NULL;
uint8_t *int_ram = NULL, *oam_io = NULL, *vidram = NULL, *int_wram = NULL, *full_int_wram = NULL, *full_vidram = NULL;

uint16_t bpalette[32], opalette[32];
uint8_t *btm[2] = { NULL }, *bwtd[2] = { NULL }, *wtm[2] = { NULL };

int hdma_on = 0, boost = 0;
