#include <stddef.h>
#include <stdint.h>

char *memory = NULL;
void *io_regs = NULL;
uint16_t _ip, _sp, _af, _bc, _de, _hl;
int card_type, rom_size, ram_size, ints_enabled = 1, want_ints_to_be = 1, lcd_on = 1;
uint32_t rdtsc_resolution = 0;
