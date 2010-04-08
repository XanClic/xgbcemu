#include <stdint.h>

#include "gbc.h"

static const int collect_overflow[4] =
{
    256, //   4096 Hz
      4, // 262144 Hz
     16, //  65536 Hz
     64  //  16384 Hz
};

#ifdef ENABLE_LINK
int link_countdown = 0;
#endif

void update_timer(int cycles_gone)
{
    static int collected = 0, vsync_collect = 0, div_collect = 0, redrawed = 0;
    #ifdef ENABLE_LINK
    static int serial_poll_collect = 0;
    #endif
    int hblank_start = 0;

    div_collect += cycles_gone;
    while (div_collect >= 64)
    {
        io_regs->div++;
        div_collect -= cycles_gone;
    }

    if (lcd_on)
    {
        vsync_collect += cycles_gone;

        io_regs->stat &= ~3;
        if (io_regs->ly >= 144)
            io_regs->stat |= 1;
        else
        {
            if (vsync_collect < 51)
                redrawed = 0;
            else if (vsync_collect < 71)
                io_regs->stat |= 2;
            else
            {
                io_regs->stat |= 3;
                if (!redrawed)
                {
                    draw_line(io_regs->ly);
                    redrawed = 1;
                }
            }
        }

        while (vsync_collect >= 114) // Eine Zeile wäre jetzt fertig
        {
            vsync_collect -= 114;

            #ifdef ENABLE_LINK
            if (!link_countdown && (++serial_poll_collect >= 10))
            {
                serial_poll_collect = 0;
                if (server != INVALID_CONN_VALUE)
                    tcp_server_poll(server);
                if (current_connection != INVALID_CONN_VALUE)
                    tcp_conn_poll(current_connection);
            }
            #endif

            hblank_start = 1;
            if (hdma_on)
                hdma_copy_16b();

            if (++io_regs->ly > 153)
                io_regs->ly = 0;
            if (io_regs->ly == 144)
            {
                io_regs->stat &= ~3;
                io_regs->stat |= 1;
                io_regs->int_flag |= INT_VBLANK;
            }

            if (io_regs->lyc == io_regs->ly)
            {
                io_regs->stat |= (1 << 2);
                if (io_regs->stat & (1 << 6))
                    io_regs->int_flag |= INT_LCDC_STAT;
            }
            else
                io_regs->stat &= ~(1 << 2);
        }

        if ((io_regs->stat & (1 << 5)) && ((io_regs->stat & 3) == 2))
            io_regs->int_flag |= INT_LCDC_STAT;
        else if ((io_regs->stat & (1 << 4)) && ((io_regs->stat & 3) == 1))
            io_regs->int_flag |= INT_LCDC_STAT;
        else if ((io_regs->stat & (1 << 3)) && hblank_start)
            io_regs->int_flag |= INT_LCDC_STAT;
    }

    if (!(io_regs->tac & (1 << 2)))
        return;

    collected += cycles_gone;
    while (collected >= collect_overflow[io_regs->tac & 3])
    {
        collected -= collect_overflow[io_regs->tac & 3];
        if (!++io_regs->tima)
        {
            io_regs->tima = io_regs->tma;
            io_regs->int_flag |= INT_TIMER;
        }
    }

    #ifdef ENABLE_LINK
    if (link_countdown)
    {
        link_countdown -= cycles_gone;
        if (link_countdown <= 0)
            link_clock();
    }
    #endif
}
