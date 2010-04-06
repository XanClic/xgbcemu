#include <stdint.h>

#include "gbc.h"

void push(uint16_t value)
{
    r_sp -= 2;
    mem_writew(r_sp, value);
}

uint16_t pop(void)
{
    uint16_t ret = mem_readw(r_sp);
    r_sp += 2;
    return ret;
}
