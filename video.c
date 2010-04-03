#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL/SDL.h>

#include "gbc.h"

static timer_t redraw_timer;
static SDL_Surface *screen;

void deinit_sdl(void);
void redraw_alrm(int signum);

static uint32_t resolutions[10];
static uint64_t old_rdtsc = 0;

// #define DISPLAY_ALL

void init_video(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "Cannot init SDL: %s\n", SDL_GetError());
        exit(1);
    }

    #ifndef DISPLAY_ALL
    screen = SDL_SetVideoMode(160, 144, 32, SDL_HWSURFACE);
    #else
    screen = SDL_SetVideoMode(256, 256, 32, SDL_HWSURFACE);
    #endif
    if (screen == NULL)
    {
        fprintf(stderr, "Cannot set video mode: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("gxemu", NULL);

    sigaction(SIGALRM, &(struct sigaction){ .sa_handler = &redraw_alrm }, NULL);

    if (timer_create(CLOCK_REALTIME, NULL, &redraw_timer))
    {
        perror("Cannot create screen update timer");
        exit(1);
    }

    struct itimerspec tmr =
    {
        .it_interval =
        {
            .tv_nsec = 1000000
        },
        .it_value =
        {
            .tv_nsec = 1000000
        }
    };

    timer_settime(redraw_timer, 0, &tmr, NULL);

    for (int i = 0; i < 10; i++)
    {
        while (!*((volatile uint32_t *)&rdtsc_resolution));

        resolutions[i] = *((volatile uint32_t *)&rdtsc_resolution);
        if (i < 9)
        {
            old_rdtsc = 0;
            __asm__ __volatile__ ("" ::: "memory");
            rdtsc_resolution = 0;
        }
    }

    printf("Determined TSC resolutions (inc/ms):\n");
    uint64_t sum = 0;
    uint32_t max = 0, min = 0xFFFFFFFF;
    for (int i = 0; i < 10; i++)
    {
        if (resolutions[i] < min)
            min = resolutions[i];
        if (resolutions[i] > max)
            max = resolutions[i];
        sum += (uint64_t)resolutions[i];
        if (i < 9)
            printf("%i ", resolutions[i]);
        else
            printf("%i\n", resolutions[i]);
    }

    sum -= min + max;

    sum /= 8;

    printf("Total resolution: %i; removed %i and %i\n", (int)sum, min, max);

    rdtsc_resolution = sum;

    timer_delete(redraw_timer);
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

        for (int rx = 0; rx < 8; rx++)
        {
            int val;
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

            ((uint32_t *)vidmem)[line + bx + rx] = pal2rgb(pal[val]);
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
    SDL_Event evt;
    int sx = io_regs->scx, sy = io_regs->scy;
    int abs_line = (line + sy) & 0xFF;

    while (SDL_PollEvent(&evt))
        if (evt.type == SDL_QUIT)
            exit(0);

    if (!(io_regs->lcdc & (1 << 7)))
        return;

    if (!(io_regs->lcdc & (1 << 0)))
        memset((uint32_t *)vidmem + abs_line * 256, 0, 256 * 4);
    else
        draw_bg_line(abs_line, 0 << 7);

    if ((io_regs->lcdc & (1 << 5)) && (io_regs->wx >= 7) && (io_regs->wx <= 166) && (io_regs->wy <= 143))
    {
        printf("WINDOW %i %i\n", io_regs->wx - 7, io_regs->wy);
        exit(0);
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

                if (val)
                    ((uint32_t *)vidmem)[abs_line * 256 + ((bx + rx) & 0xFF)] = pal2rgb(pal[val]);
            }
        }
    }

    if (io_regs->lcdc & (1 << 0))
        draw_bg_line(abs_line, 1 << 7);

    if (line == 143)
    {
        SDL_LockSurface(screen);
        #ifndef DISPLAY_ALL
        for (int y = 0; y < 144; y++)
        {
            for (int x = 0; x < 160; x++)
            {
                int px = (x + io_regs->scx) & 0xFF;
                int py = (y + io_regs->scy) & 0xFF;
                uint32_t col = ((uint32_t *)vidmem)[py * 256 + px];
                ((uint32_t *)screen->pixels)[y * 160 + x] = SDL_MapRGB(screen->format, (col & 0xFF0000) >> 16, (col & 0xFF00) >> 8, col & 0xFF);
            }
        }
        #else
        for (int y = 0; y < 256; y++)
        {
            for (int x = 0; x < 256; x++)
            {
                uint32_t col = ((uint32_t *)vidmem)[y * 256 + x];
                ((uint32_t *)screen->pixels)[y * 256 + x] = SDL_MapRGB(screen->format, (col & 0xFF0000) >> 16, (col & 0xFF00) >> 8, col & 0xFF);
            }
        }
        #endif
        SDL_UnlockSurface(screen);

        SDL_UpdateRect(screen, 0, 0, 0, 0);
    }

    // redrawing--;
}

void redraw_alrm(int signum)
{
    if (signum != SIGALRM)
        return;
    if (!rdtsc_resolution)
    {
        if (!old_rdtsc)
        {
            uint32_t lo, hi;
            __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
            old_rdtsc = (uint64_t)lo | ((uint64_t)hi << 32);
        }
        else
        {
            uint32_t lo, hi;
            __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
            rdtsc_resolution = ((uint64_t)lo | ((uint64_t)hi << 32)) - old_rdtsc;
            printf("Res: 0x%08X\n", rdtsc_resolution);
        }
    }
    // redraw();
}
