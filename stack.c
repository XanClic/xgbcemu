#include <stdint.h>

#include "gbc.h"

void push(uint16_t value)
{
    sp -= 2;
    *((uint16_t *)&memory[sp]) = value;
}

uint16_t pop(void)
{
    uint16_t ret = *((uint16_t *)&memory[sp]);
    sp += 2;
    return ret;
}
