#include <stdio.h>
#include <stdlib.h>

#include "gbc.h"

// #define DUMP

void generate_interrupts(void)
{
    int cause = io_regs->int_enable & io_regs->int_flag;

    if (!ints_enabled || !cause)
        return;

    want_ints_to_be = 0;
    ints_enabled = 0;
    push(r_ip);

    if (cause & INT_P10_P13) // Hi-Lo of P10-P13
    {
        #ifdef DUMP
        os_print("Issuing P10/P13 hi-lo\n");
        #endif
        cause = 1 << 4;
    }
    else if (cause & INT_SERIAL) // Serial transfer completed
    {
        #ifdef DUMP
        os_print("Issuing serial\n");
        #endif
        cause = 1 << 3;
    }
    else if (cause & INT_TIMER) // Timer overflow
    {
        #ifdef DUMP
        os_print("Issuing timer\n");
        #endif
        cause = 1 << 2;
    }
    else if (cause & INT_LCDC_STAT) // LCDC STAT
    {
        #ifdef DUMP
        os_print("Issuing LCDC\n");
        #endif
        cause = 1 << 1;
    }
    else if (cause & INT_VBLANK) // V Blank
    {
        #ifdef DUMP
        os_print("Issuing VBlank\n");
        #endif
        cause = 1 << 0;
    }
    else
    {
        #ifdef DUMP
        os_print("Unknown interrupt cause 0x%02X\n", cause);
        #endif
        return;
    }

    interrupt_issued = 1;

    io_regs->int_flag &= ~cause;

    switch (cause)
    {
        case INT_VBLANK:
            r_ip = 0x40;
            break;
        case INT_LCDC_STAT:
            r_ip = 0x48;
            break;
        case INT_TIMER:
            r_ip = 0x50;
            break;
        case INT_SERIAL:
            r_ip = 0x58;
            break;
        case INT_P10_P13:
            r_ip = 0x60;
    }
}
