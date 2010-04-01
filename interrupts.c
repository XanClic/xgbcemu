#include <stdio.h>
#include <stdlib.h>

#include "gbc.h"

#define DUMP

void generate_interrupts(void)
{
    int cause = io_regs->int_enable & io_regs->int_flag;

    if (!ints_enabled || !cause)
        return;

    ints_enabled = 0;
    push(ip);

    if (cause & INT_P10_P13) // Hi-Lo of P10-P13
        cause = 1 << 4;
    else if (cause & INT_SERIAL) // Serial transfer completed
        cause = 1 << 3;
    else if (cause & INT_TIMER) // Timer overflow
        cause = 1 << 2;
    else if (cause & INT_LCDC_STAT) // LCDC STAT
        cause = 1 << 1;
    else if (cause & INT_VBLANK) // V Blank
        cause = 1 << 0;
    else
    {
        fprintf(stderr, "Unknown interrupt cause 0x%02X\n", cause);
        exit(1);
    }

    #ifdef DUMP
    printf("Generating interrupt because of 0x%02X\n", cause);
    #endif

    interrupt_issued = 1;

    io_regs->int_flag &= ~cause;

    switch (cause)
    {
        case INT_VBLANK:
            ip = 0x40;
            break;
        case INT_LCDC_STAT:
            ip = 0x48;
            break;
        case INT_TIMER:
            ip = 0x50;
            break;
        case INT_SERIAL:
            ip = 0x58;
            break;
        case INT_P10_P13:
            ip = 0x60;
    }
}
