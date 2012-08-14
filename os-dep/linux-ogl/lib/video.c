#define GL_GLEXT_PROTOTYPES

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL/SDL.h>

#include "gbc.h"

// #define DISPLAY_ALL

#define LMSOPF // load most stuff once per frame

extern int lgl_scr_width, lgl_scr_height, lgl_multiplier;

extern GLuint lgl_vidram_buffer, lgl_bg_prg, lgl_obj_prg;
extern GLuint lgl_y, lgl_scx, lgl_scy, lgl_wnd, lgl_lcdc, lgl_obj_y, lgl_obj_lcdc;

void draw_line(int line)
{
    if (!lcd_on)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (line == 143)
            os_handle_events();
        return;
    }


#ifdef LMSOPF
    if (!line)
    {
#endif
    // glBindBuffer(GL_TEXTURE_BUFFER, lgl_vidram_buffer);
    glBufferSubData(GL_TEXTURE_BUFFER, 0, 2 * 8192, full_vidram);
    // glActiveTexture(GL_TEXTURE1);
    // god may bless OpenGL for that data type
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 8, 2, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, _palettes);
#ifdef LMSOPF
    }
#endif

    struct
    {
        uint8_t y, x;
        uint8_t num;
        uint8_t flags;
    } __attribute__((packed)) *oam = (void *)oam_io;


    if (gbc_mode || (io_regs->lcdc & (1 << 0)))
    {
        glUseProgram(lgl_bg_prg);

        glUniform1i(lgl_y, line);
        glUniform1i(lgl_scx, io_regs->scx);
        glUniform1i(lgl_scy, io_regs->scy);

        // Window enable
        if (io_regs->lcdc & (1 << 5))
            glUniform2i(lgl_wnd, io_regs->wx - 7, io_regs->wy);
        else
            glUniform2i(lgl_wnd, 0xFF, 0xFF);

        glUniform1i(lgl_lcdc, io_regs->lcdc);

        glDrawArrays(GL_LINES, 0, 2);
    }


    if (io_regs->lcdc & (1 << 1))
    {
        // max. 10 sprites per line
        int sprites_in_this_line = 0, sprites_to_draw = 0;

        int oam_data[20][4];

        // 8x16
        bool large_objs = io_regs->lcdc & (1 << 2);

        for (int i = 0; i < 40; i++)
        {
            int y_s = oam[i].y - 16;
            int y_e = y_s + (large_objs ? 16 : 8);

            if ((line < y_s) || (line >= y_e) || (sprites_in_this_line >= 10))
                continue;

            sprites_in_this_line++;

            int x = oam[i].x - 8;

            if ((x < -7) || (x >= 160))
                continue;

            // save backwards, so that the sprite with the highest priority is drawn last (incompatible with DMG, but, oh well.)
            int *this_obj_data = oam_data[2 * (9 - sprites_to_draw++)];

            this_obj_data[0] = x;
            this_obj_data[1] = y_s;
            this_obj_data[2] = oam[i].num & (large_objs ? 0xFE : 0xFF); // 8x16: ignore LSb
            this_obj_data[3] = oam[i].flags;

            memcpy(this_obj_data + 4, this_obj_data, 4 * sizeof(int));
        }

        if (sprites_to_draw)
        {
            glUseProgram(lgl_obj_prg);
            glUniform1i(lgl_obj_y, line);
            glUniform1i(lgl_obj_lcdc, io_regs->lcdc);

            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(int) * 2 * sprites_to_draw, oam_data[2 * (10 - sprites_to_draw)]);

            glDrawArrays(GL_LINES, 0, sprites_to_draw * 2);
        }
    }


    if (line == 143)
    {
        os_handle_events();

        #ifdef DISPLAY_ALL
        for (int rem = 144; rem < 256; rem++)
            draw_line(rem);
        #endif

        SDL_GL_SwapBuffers();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}
