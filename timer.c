#include <stdint.h>

#include "gbc.h"

static const int collect_overflow[4] =
{
    244141, //   4096 Hz
      3815, // 262144 Hz
     15259, //  65536 Hz
     61035  //  16384 Hz
};

void update_timer(uint64_t us_gone)
{
    static int collected = 0, vsync_collect = 0;

    if (lcd_on)
    {
        vsync_collect += us_gone;
        if (vsync_collect >= 108714) // Eine Zeile wäre jetzt fertig
        {
            vsync_collect -= 108714;

            if (io_regs->stat & (1 << 3))
                io_regs->int_flag |= INT_LCDC_STAT;

            if (++io_regs->ly > 153)
                io_regs->ly = 0;
            if (io_regs->ly == 144)
            {
                io_regs->int_flag |= INT_VBLANK;
                if (io_regs->stat & (1 << 4))
                    io_regs->int_flag |= INT_LCDC_STAT;
            }

            if (io_regs->lyc == io_regs->ly)
            {
                io_regs->stat |= (1 << 2);
                if (io_regs->stat & (1 << 6))
                    io_regs->int_flag |= INT_LCDC_STAT;
            }
            else
                io_regs->stat &= ~(1 << 2);

            generate_interrupts();
        }
    }

    if (!(io_regs->tac & (1 << 2)))
        return;

    collected += us_gone;
    if (collected >= collect_overflow[io_regs->tac & 3])
    {
        collected -= collect_overflow[io_regs->tac & 3];
        if (!++io_regs->tima)
        {
            io_regs->tima = io_regs->tma;
            io_regs->int_flag = INT_TIMER;
            generate_interrupts();
        }
    }
}
