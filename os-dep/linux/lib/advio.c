#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include "gbc.h"

// Advanced I/O support for Linux (using SDL)

static SDL_Surface *screen;
static int scr_width, scr_height, multiplier;

extern uint64_t total_cycles_gone;

struct ReplayEvent {
    uint64_t timestamp;
    int keystate;
};

extern bool replay;
static struct ReplayEvent *events;
static const struct ReplayEvent *next_event;
static bool recording_events;
static uint64_t event_count, event_capacity;

static void save_replay(void)
{
    FILE *replay_fp = fopen("replay", "w");
    if (!replay_fp) {
        return;
    }

    fprintf(replay_fp, "%" PRIu64 "\n", event_count);
    for (uint64_t i = 0; i < event_count; i++) {
        fprintf(replay_fp, "%" PRIu64 " %i\n", events[i].timestamp, events[i].keystate);
    }
    fclose(replay_fp);
}

void os_open_screen(int width, int height, int mult)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "Cannot init SDL: %s\n", SDL_GetError());
        exit(1);
    }

    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(width * mult, height * mult, 32, SDL_HWSURFACE);
    if (screen == NULL)
    {
        fprintf(stderr, "Cannot set video mode: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("xgbcemu", NULL);

    scr_width = width;
    scr_height = height;
    multiplier = mult;

    if (!replay) {
        return;
    }

    atexit(save_replay);

    FILE *replay_fp = fopen("replay", "r");
    if (!replay_fp) {
        recording_events = true;
        return;
    }

    fscanf(replay_fp, "%" PRIu64 "\n", &event_count);

    if (!event_count) {
        recording_events = true;
        fclose(replay_fp);
        return;
    }

    events = calloc(event_count, sizeof(*events));
    if (!events) {
        fclose(replay_fp);
        return;
    }
    event_capacity = event_count;

    for (uint64_t i = 0; i < event_count; i++) {
        fscanf(replay_fp, "%" PRIu64 " %i\n",
               &events[i].timestamp, &events[i].keystate);
    }
    fclose(replay_fp);

    next_event = events;
}

void os_handle_events(void)
{
    SDL_Event evt;
    int new_keystate = keystates;

    while (SDL_PollEvent(&evt))
    {
        switch (evt.type)
        {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                switch (evt.key.keysym.sym)
                {
                    case SDLK_a:
                    case SDLK_SPACE:
                        new_keystate |= VK_A;
                        break;
                    case SDLK_b:
                    case SDLK_LCTRL:
                        new_keystate |= VK_B;
                        break;
                    case SDLK_RETURN:
                        new_keystate |= VK_START;
                        break;
                    case SDLK_s:
                        new_keystate |= VK_SELECT;
                        break;
                    case SDLK_LEFT:
                        new_keystate |= VK_LEFT;
                        break;
                    case SDLK_RIGHT:
                        new_keystate |= VK_RIGHT;
                        break;
                    case SDLK_UP:
                        new_keystate |= VK_UP;
                        break;
                    case SDLK_DOWN:
                        new_keystate |= VK_DOWN;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (evt.key.keysym.sym)
                {
                    case SDLK_a:
                    case SDLK_SPACE:
                        new_keystate &= ~VK_A;
                        break;
                    case SDLK_b:
                    case SDLK_LCTRL:
                        new_keystate &= ~VK_B;
                        break;
                    case SDLK_RETURN:
                        new_keystate &= ~VK_START;
                        break;
                    case SDLK_s:
                        new_keystate &= ~VK_SELECT;
                        break;
                    case SDLK_LEFT:
                        new_keystate &= ~VK_LEFT;
                        break;
                    case SDLK_RIGHT:
                        new_keystate &= ~VK_RIGHT;
                        break;
                    case SDLK_UP:
                        new_keystate &= ~VK_UP;
                        break;
                    case SDLK_DOWN:
                        new_keystate &= ~VK_DOWN;
                        break;
                    case SDLK_TAB:
                        save_to_disk();
                        break;
                    case SDLK_LSHIFT:
                        boost ^= 1;
                        break;
                    default:
                        break;
                }
        }
    }

    if (next_event) {
        new_keystate = keystates;
    }

    if (new_keystate != keystates && recording_events) {
        if (event_count == event_capacity) {
            event_capacity = (event_capacity + 2) * 3 / 2;
            // multiplication overflow
            events = realloc(events, event_capacity * sizeof(*events));
        }

        struct ReplayEvent *replay_evt = events + event_count++;
        replay_evt->timestamp = total_cycles_gone;
        replay_evt->keystate = new_keystate;
    }

    if (next_event) {
        while (next_event && next_event->timestamp <= total_cycles_gone) {
            new_keystate = next_event->keystate;
            if ((uint64_t)(++next_event - events) == event_count) {
                next_event = NULL;
                recording_events = true;
            }
        }
    }

    if (new_keystate != keystates)
    {
        keystates = new_keystate;
        update_keyboard();
    }
}

void os_draw_line(int offx, int offy, int line)
{
    int py = ((line + offy) & 0xFF) << 8;
    int rline = line;

    line *= scr_width * multiplier * multiplier;

    SDL_LockSurface(screen);

    for (int x = 0; x < scr_width; x++)
    {
        int px = (x + offx) & 0xFF;
        uint32_t col = ((uint32_t *)vidmem)[py + px];
        if (multiplier == 1)
            ((uint32_t *)screen->pixels)[line + x] = SDL_MapRGB(screen->format, (col & 0xFF0000) >> 16, (col & 0xFF00) >> 8, col & 0xFF);
        else
        {
            for (int ax = 0; ax < multiplier; ax++)
                for (int aline = 0; aline < multiplier; aline++)
                    ((uint32_t *)screen->pixels)[line + (aline * scr_width + x) * multiplier + ax] = SDL_MapRGB(screen->format, (col & 0xFF0000) >> 16, (col & 0xFF00) >> 8, col & 0xFF);
        }
    }

    SDL_UnlockSurface(screen);

    if (rline == scr_height - 1)
        SDL_UpdateRect(screen, 0, 0, 0, 0);
}
