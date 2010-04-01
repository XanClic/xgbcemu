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

void init_video(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "Cannot init SDL: %s\n", SDL_GetError());
        exit(1);
    }

    atexit(deinit_sdl);

    screen = SDL_SetVideoMode(160, 144, 16, SDL_HWSURFACE);
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
}

void redraw(void)
{
    // SDL_UpdateRect(screen, 0, 0, 0, 0);
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
    redraw();
}

void deinit_sdl(void)
{
    timer_delete(redraw_timer);

    SDL_WM_SetCaption("gxemu - Finished", NULL);

    SDL_Quit();
}
