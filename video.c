#include <string.h>

#include "gbc.h"

// #define DISPLAY_ALL

void init_video(void)
{
    #ifdef DISPLAY_ALL
    os_open_screen(256, 256);
    #else
    os_open_screen(160, 144);
    #endif
}

static void draw_bg_line(int line, int bit7val)
{
    int by = line & 0xF8, ry = line & 0x07;
    int tile = by * 4;

    line *= 256;

    for (int bx = 0; bx < 256; bx += 8)
    {
        int flags = btm[1][tile];

        if ((flags & (1 << 7)) != bit7val)
        {
            tile++;
            continue;
        }

        uint8_t *tdat;
        int vbank = !!(flags & (1 << 3));
        uint16_t *pal = &bpalette[(flags & 7) * 4];

        if (bwtd[0] == (uint8_t *)&full_vidram[0x0000])
            tdat = &bwtd[vbank][(unsigned)btm[0][tile] * 16];
        else
            tdat = &bwtd[vbank][(int)(int8_t)btm[0][tile] * 16];

        int b1, b2;

        if (flags & (1 << 6))
        {
            b1 = tdat[(7 - ry) * 2];
            b2 = tdat[(7 - ry) * 2 + 1];
        }
        else
        {
            b1 = tdat[ry * 2];
            b2 = tdat[ry * 2 + 1];
        }

        for (int rx = 0; rx < 8; rx++)
        {
            int val, mask;

            if (flags & (1 << 5))
                mask = 1 << rx;
            else
                mask = 1 << (7 - rx);

            val = !!(b1 & mask) + !!(b2 & mask) * 2;

            ((uint32_t *)vidmem)[line + bx + rx] = pal2rgb(pal[val]) | (val << 24);
        }

        tile++;
    }
}

void draw_line(int line)
{
    struct
    {
        uint8_t y, x;
        uint8_t num;
        uint8_t flags;
    } __attribute__((packed)) *oam = (void *)oam_io;
    int sx = io_regs->scx, sy = io_regs->scy;
    int abs_line = (line + sy) & 0xFF;

    if (!(io_regs->lcdc & (1 << 7)))
        return;

    if (!(io_regs->lcdc & (1 << 0)))
        memset((uint32_t *)vidmem + abs_line * 256, 0, 256 * 4);
    else
        draw_bg_line(abs_line, 0 << 7);

    if ((io_regs->lcdc & (1 << 5)) && (io_regs->wx >= 7) && (io_regs->wx <= 166) && (io_regs->wy <= 143) && (io_regs->wy <= line))
    {
        int wx = io_regs->wx + sx - 7, wy = io_regs->wy + sy;
        int yoff = line - wy;
        int by = yoff & 0xF8, ry = yoff & 0x07;
        int tile = by * 4;

        for (int bx = 0; bx < 256; bx += 8)
        {
            int flags = wtm[1][tile];

            if (bx + wx >= 256)
                break;

            uint8_t *tdat;
            int vbank = !!(flags & (1 << 3));
            uint16_t *pal = &bpalette[(flags & 7) * 4];

            if (bwtd[0] == (uint8_t *)&full_vidram[0x0000])
                tdat = &bwtd[vbank][(unsigned)wtm[0][tile] * 16];
            else
                tdat = &bwtd[vbank][(int)(int8_t)wtm[0][tile] * 16];

            for (int rx = 0; rx < 8; rx++)
            {
                int val;

                if (bx + rx + wx >= 256)
                    break;

                switch (flags & (3 << 5))
                {
                    case (0 << 5):
                        val = !!(tdat[ry * 2] & (1 << (7 - rx)));
                        val += !!(tdat[ry * 2 + 1] & (1 << (7 - rx))) << 1;
                        break;
                    case (1 << 5):
                        val = !!(tdat[ry * 2] & (1 << rx));
                        val += !!(tdat[ry * 2 + 1] & (1 << rx)) << 1;
                        break;
                    case (2 << 5):
                        val = !!(tdat[(7 - ry) * 2] & (1 << (7 - rx)));
                        val += !!(tdat[(7 - ry) * 2 + 1] & (1 << (7 - rx))) << 1;
                        break;
                    default:
                        val = !!(tdat[(7 - ry) * 2] & (1 << rx));
                        val += !!(tdat[(7 - ry) * 2 + 1] & (1 << rx)) << 1;
                }

                ((uint32_t *)vidmem)[abs_line * 256 + bx + rx + wx] = pal2rgb(pal[val]);
            }

            tile++;
        }
    }

    if (io_regs->lcdc & (1 << 1))
    {
        int obj_height = (io_regs->lcdc & (1 << 2)) ? 16 : 8;
        int count = 0;

        for (int sprite = 40; sprite >= 0; sprite--)
        {
            uint8_t *tdat;
            int bx = oam[sprite].x, by = oam[sprite].y, flags = oam[sprite].flags;
            uint16_t *pal = &opalette[(flags & 7) * 4];

            bx -= 8;
            by -= 16;

            if ((by > line) || (by + obj_height <= line))
                continue;

            if (count++ >= 10)
                break;

            if ((bx <= -8) || (bx >= 160))
                continue;

            int ry = line - by;

            if (flags & (1 << 6))
                ry = (obj_height - 1) - ry;

            if (obj_height == 8)
            {
                if (!(flags & (1 << 3)))
                    tdat = &full_vidram[0x0000 + oam[sprite].num * 16];
                else
                    tdat = &full_vidram[0x2000 + oam[sprite].num * 16];
            }
            else
            {
                if (!(flags & (1 << 3)))
                    tdat = &full_vidram[0x0000 + (oam[sprite].num & 0xFE) * 16];
                else
                    tdat = &full_vidram[0x2000 + (oam[sprite].num & 0xFE) * 16];
            }

            bx += sx;
            by += sy;

            for (int rx = 0; rx < 8; rx++)
            {
                int val, bmask;

                if (flags & (1 << 5))
                    bmask = 1 << rx;
                else
                    bmask = 1 << (7 - rx);

                val = !!(tdat[ry * 2] & bmask);
                val += !!(tdat[ry * 2 + 1] & bmask) << 1;

                if (val && !(flags & (1 << 7)))
                    ((uint32_t *)vidmem)[abs_line * 256 + ((bx + rx) & 0xFF)] = pal2rgb(pal[val]);
                else if (val)
                {
                    if (!(((uint32_t *)vidmem)[abs_line * 256 + ((bx + rx) & 0xFF)] & 0xFF000000))
                        ((uint32_t *)vidmem)[abs_line * 256 + ((bx + rx) & 0xFF)] = pal2rgb(pal[val]);
                }
            }
        }
    }

    if (io_regs->lcdc & (1 << 0))
        draw_bg_line(abs_line, 1 << 7);

    if (line == 143)
    {
        os_handle_events();

        #ifdef DISPLAY_ALL
        for (int rem = 144; rem < 256; rem++)
            draw_line(rem);
        #endif
    }

    os_draw_line(sx, sy, line);
}
