#include <stdio.h>
#include <stdlib.h>

#include <cdi/bios.h>
#include <cdi/mem.h>

#include "gbc.h"

// Advanced I/O support for CDI systems (using mode 13h)

static uint8_t *screen = NULL;
static int scr_width;

void os_open_screen(int width, int height)
{
    struct cdi_mem_area *scr = cdi_mem_map(0xA0000, 320 * 200);

    if (scr == NULL)
    {
        fprintf(stderr, "Could not map 0xA0000.\n");
        exit(1);
    }

    screen = scr->vaddr;

    if (cdi_bios_int10(&(struct cdi_bios_registers){ .ax = 0x13 }, NULL))
    {
        fprintf(stderr, "Could not switch to mode 13h.\n");
        exit(1);
    }

    scr_width = width;
}

static void leave_13h(void)
{
    cdi_bios_int10(&(struct cdi_bios_registers){ .ax = 0x1114, .bx = 0 }, NULL);
}

void custom_exit(int num)
{
    leave_13h();
    exit(num);
}

void os_handle_events(void)
{
    int new_keystate = 0;

    clearerr(stdin);

    while (!feof(stdin))
    {
        switch (getchar())
        {
            case 'a':
            case 'A':
                new_keystate |= VK_A;
                break;
            case 'b':
            case 'B':
                new_keystate |= VK_B;
                break;
            case '\n':
                new_keystate |= VK_START;
                break;
            case 's':
            case 'S':
                new_keystate |= VK_SELECT;
                break;
            case ' ':
                save_to_disk();
                break;
            // case LSHIFT: boost ^= 1; break; 
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

    line *= 320;

    for (int x = 0; x < scr_width; x++)
    {
        int px = (x + offx) & 0xFF;
        uint32_t col = ((uint32_t *)vidmem)[py + px];
        screen[line + x] = col & 0xFF;
    }
}
