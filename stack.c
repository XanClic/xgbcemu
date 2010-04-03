#include <stdint.h>

#include "gbc.h"

void push(uint16_t value)
{
    sp -= 2;
    mem_writew(sp, value);
}

uint16_t pop(void)
{
    uint16_t ret = mem_readw(sp);
    sp += 2;
    return ret;
}
