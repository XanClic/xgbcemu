#include "gbc.h"

static int keys_were = 0;

void update_keyboard(void)
{
    int diff_key = keystates ^ keys_were;
    int value = io_regs->p1 & (KEY_DIR | KEY_OTH);
    int relevant = 0;

    if (!(value & KEY_DIR))
    {
        value |= ~((keystates & 0xF0) >> 4);
        relevant |= 0xF0;
    }
    if (!(value & KEY_OTH))
    {
        value |= ~(keystates & 0x0F);
        relevant |= 0x0F;
    }

    io_regs->p1 = value;

    diff_key &= relevant;
    if (keystates & diff_key)
        io_regs->int_flag |= INT_P10_P13;

    keys_were = keystates;
}
