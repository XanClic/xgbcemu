#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cdi/bios.h>
#include <cdi/io.h>
#include <cdi/mem.h>
#include <cdi/misc.h>

#include "gbc.h"

// Advanced I/O support for CDI systems (using mode 13h)

static uint8_t *screen = NULL;
static int scr_width, last_key_update = -1, key_through = 0;

uint32_t *old_vga_palette;

void os_open_screen(int width, int height)
{
    struct cdi_mem_area *scr = cdi_mem_map(0xA0000, 320 * 200);

    if (scr == NULL)
    {
        fprintf(stderr, "Could not map 0xA0000.\n");
        exit(1);
    }

    screen = scr->vaddr;

    if (cdi_ioports_alloc(0x3C8, 2))
    {
        cdi_mem_free(scr);
        fprintf(stderr, "Could not allocate palette I/O ports!\n");
        exit(1);
    }

    if (cdi_bios_int10(&(struct cdi_bios_registers){ .ax = 0x13 }, NULL))
    {
        fprintf(stderr, "Could not switch to mode 13h.\n");
        exit(1);
    }

    scr_width = width;

    old_vga_palette = malloc(1024);

    for (int color = 0x00; color <= 0xFF; color++)
    {
        cdi_outb(0x3C8, color);
        old_vga_palette[color] = cdi_inb(0x3C9);
        old_vga_palette[color] |= cdi_inb(0x3C9) << 8;
        old_vga_palette[color] |= cdi_inb(0x3C9) << 16;
    }

    cdi_outb(0x3C8, 0);
    for (int color = 0x00; color <= 0xFF; color++)
    {
        cdi_outb(0x3C9, ((color & 0xE0) >> 5) *  9);
        cdi_outb(0x3C9, ((color & 0x1C) >> 2) *  9);
        cdi_outb(0x3C9,  (color & 0x03)       * 21);
    }

    memset(screen, 0xE8, 64000);
}

static void leave_13h(void)
{
    cdi_outb(0x3C8, 0);
    for (int color = 0x00; color <= 0xFF; color++)
    {
        cdi_outb(0x3C9,  old_vga_palette[color] & 0x0000FF       );
        cdi_outb(0x3C9, (old_vga_palette[color] & 0x00FF00) >>  8);
        cdi_outb(0x3C9, (old_vga_palette[color] & 0xFF0000) >> 16);
    }

    cdi_bios_int10(&(struct cdi_bios_registers){ .ax = 0x1114, .bx = 0 }, NULL);
}

void custom_exit(int num)
{
    if (screen != NULL)
        leave_13h();
    exit(num);
}

void os_handle_events(void)
{
    int new_keystate = keystates;
    int input = 0;

    if (fread(&input, 1, 1, stdin))
    {
        switch (input)
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
            case 'q':
            case 'Q':
                custom_exit(0);
                break;
            case ' ':
                save_to_disk();
                break;
            case 'f':
            case 'F':
                boost ^= 1;
                break;
            case 'i':
            case 'I':
                new_keystate |= VK_UP;
                break;
            case 'j':
            case 'J':
                new_keystate |= VK_LEFT;
                break;
            case 'k':
            case 'K':
                new_keystate |= VK_DOWN;
                break;
            case 'l':
            case 'L':
                new_keystate |= VK_RIGHT;
                break;
        }
    }

    if (new_keystate != keystates)
    {
        last_key_update = (unsigned)(io_regs->lyc - 1) & 0xFF;
        if (last_key_update > 143)
            last_key_update = 143;
        key_through = 10;
        keystates = new_keystate;
        update_keyboard();
    }
}

void os_draw_line(int offx, int offy, int line)
{
    int py = ((line + offy) & 0xFF) << 8;

    if (line == last_key_update)
    {
        if (!--key_through)
        {
            last_key_update = -1;
            keystates = 0;
            update_keyboard();
        }
    }

    line = (line + 28) * 320;

    for (int x = 0; x < scr_width; x++)
    {
        int px = (x + offx) & 0xFF;
        uint32_t col = ((uint32_t *)vidmem)[py + px];
        screen[line + x + 80] = (((col & 0xFF0000) >> 16) & 0xE0) | (((col & 0xFF00) >> 11) & 0x1C) | ((col & 0xFF) >> 6);
    }
}
