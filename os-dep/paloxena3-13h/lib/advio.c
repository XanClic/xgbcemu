#include <fcntl.h>
#include <paloxena3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vm86.h>

#include "gbc.h"

static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ __volatile__ ("inb %%dx,%%al" : "=a"(val) : "d"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__ ("outb %%al,%%dx" :: "a"(val), "d"(port));
}

// Advanced I/O support for CDI systems (using mode 13h)

static uint8_t *screen = NULL;
static int scr_width, last_key_update = -1, key_through = 0;
static void *tcA0000, *tcB8000;

extern int frameskip_skip, frameskip_draw;

uint32_t *old_vga_palette;

void os_open_screen(int width, int height, int multiplier)
{
    height = 0;

    multiplier = 0;

    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

    tcA0000 = malloc(320 * 200);
    tcB8000 = malloc(80 * 25 * 2);

    screen = map_framebuffer(0xB8000, 80 * 25 * 2);
    if (screen == NULL)
    {
        printf("Could not map 0xB8000.\n");
        exit(1);
    }
    memcpy(tcB8000, screen, 80 * 25 * 2);

    create_vm86_session();

    vm86_set_registers(&(struct vm86_registers){ .eax = 0x13 });
    vm86_int(0x10);

    screen = map_framebuffer(0xA0000, 320 * 200);
    if (screen == NULL)
    {
        printf("Could not map 0xA0000.\n");
        custom_exit(1);
    }
    memcpy(tcA0000, screen, 320 * 200);

    scr_width = width;

    old_vga_palette = malloc(1024);

    for (int color = 0x00; color <= 0xFF; color++)
    {
        outb(0x3C8, color);
        old_vga_palette[color] = inb(0x3C9);
        old_vga_palette[color] |= inb(0x3C9) << 8;
        old_vga_palette[color] |= inb(0x3C9) << 16;
    }

    outb(0x3C8, 0);
    for (int color = 0x00; color <= 0xFF; color++)
    {
        outb(0x3C9, ((color & 0xE0) >> 5) *  9);
        outb(0x3C9, ((color & 0x1C) >> 2) *  9);
        outb(0x3C9,  (color & 0x03)       * 21);
    }

    memset(screen, 0xFF, 64000);
}

static void leave_13h(void)
{
    outb(0x3C8, 0);
    for (int color = 0x00; color <= 0xFF; color++)
    {
        outb(0x3C9,  old_vga_palette[color] & 0x0000FF       );
        outb(0x3C9, (old_vga_palette[color] & 0x00FF00) >>  8);
        outb(0x3C9, (old_vga_palette[color] & 0xFF0000) >> 16);
    }

    vm86_set_registers(&(struct vm86_registers){ .eax = 0x03 });
    vm86_int(0x10);

    vm86_set_registers(&(struct vm86_registers){ .eax = 0x1114 });
    vm86_int(0x10);
}

void custom_exit(int num)
{
    if (screen != NULL)
    {
        memcpy(screen, tcA0000, 320 * 200);
        leave_13h();
        destroy_vm86_session();
        screen = map_framebuffer(0xB8000, 80 * 25 * 2);
        if (screen != NULL)
            memcpy(screen, tcB8000, 80 * 25 * 2);
    }
    exit(num);
}

void os_handle_events(void)
{
    int new_keystate = keystates;
    int input = getchar();

    if (input)
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
                increase_frameskip();
                break;
            case 'F':
                decrease_frameskip();
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

    if (!line)
    {
        for (int i = 0; i < frameskip_skip; i++)
            screen[180 * 320 + 80 + i * 2] = 0xE8;
        for (int i = frameskip_skip; i < 16; i++)
            screen[180 * 320 + 80 + i * 2] = 0xFF;
        for (int i = 0; i < frameskip_draw; i++)
            screen[182 * 320 + 80 + i * 2] = 0xE8;
        for (int i = frameskip_draw; i < 16; i++)
            screen[182 * 320 + 80 + i * 2] = 0xFF;
    }

    line = (line + 28) * 320 + 80;

    for (int x = 0; x < scr_width; x++)
    {
        int px = (x + offx) & 0xFF;
        uint32_t col = ((uint32_t *)vidmem)[py + px];
        screen[line + x] = (((col & 0xFF0000) >> 16) & 0xE0) | (((col & 0xFF00) >> 11) & 0x1C) | ((col & 0xFF) >> 6);
    }
}
