#define GL_GLEXT_PROTOTYPES

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include "gbc.h"

// Advanced I/O support for Linux (using SDL)

static SDL_Surface *screen;
int lgl_scr_width, lgl_scr_height, lgl_multiplier;

GLuint lgl_vidram_buffer, lgl_vidram_tex, lgl_oam_tex, lgl_pal_tex, lgl_bg_prg, lgl_obj_prg, lgl_vertex_data, lgl_vertex_oam;
GLuint lgl_y, lgl_scx, lgl_scy, lgl_wnd, lgl_lcdc, lgl_obj_lcdc, lgl_obj_y;


const char *shader_type(GLenum type)
{
    switch (type)
    {
        case GL_VERTEX_SHADER:   return "vertex";
        case GL_FRAGMENT_SHADER: return "fragment";
        default:                 return "unknown";
    }
}

GLuint create_shader(GLenum type, const char *file)
{
    GLuint sh = glCreateShader(type);

    FILE *fp = fopen(file, "r");
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp); // GL wants an int, so give it one (and hope the shader won't exceed 2 GB)
    rewind(fp);

    char *data = malloc(sz);
    fread(data, 1, sz, fp);
    fclose(fp);

    glShaderSource(sh, 1, (const GLchar **)&data, &sz);
    free(data);


    GLint status;
    glCompileShader(sh);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &status);

    if (!status)
    {
        int illen;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &illen);

        if (illen <= 1)
            fprintf(stderr, "Error compiling %s shader (from %s), reason unknown.\n", shader_type(type), file);
        else
        {
            char *msg = malloc(illen + 1);

            glGetShaderInfoLog(sh, illen, NULL, msg);
            msg[illen] = 0;

            fprintf(stderr, "Error compile %s shader (from %s): %s\n", shader_type(type), file, msg);

            free(msg);
        }

        assert(0);
    }


    return sh;
}

GLuint create_program(const char *vfile, const char *ffile)
{
    GLuint vsh = create_shader(GL_VERTEX_SHADER,   vfile);
    GLuint fsh = create_shader(GL_FRAGMENT_SHADER, ffile);

    GLuint prg = glCreateProgram();
    glAttachShader(prg, vsh);
    glAttachShader(prg, fsh);

    GLint status;
    glLinkProgram(prg);
    glGetProgramiv(prg, GL_LINK_STATUS, &status);
    assert(status);

    glDeleteShader(vsh);
    glDeleteShader(fsh);

    return prg;
}

void os_open_screen(int width, int height, int mult)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "Cannot init SDL: %s\n", SDL_GetError());
        exit(1);
    }

    atexit(SDL_Quit);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    screen = SDL_SetVideoMode(width * mult, height * mult, 32, SDL_OPENGL | SDL_DOUBLEBUF);
    if (screen == NULL)
    {
        fprintf(stderr, "Cannot set video mode: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("xgbcemu", NULL);

    lgl_scr_width = mult * width;
    lgl_scr_height = mult * height;
    lgl_multiplier = mult;


    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, .1f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_BUFFER);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_3D);


    // cleared screen is white
    glClearColor(1.f, 1.f, 1.f, 0.f);

    glClearDepth(1.f);


    // Buffer for video memory
    glGenBuffers(1, &lgl_vidram_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, lgl_vidram_buffer);
    // Two banks of 8k each
    glBufferData(GL_TEXTURE_BUFFER, 2 * 8192, NULL, GL_DYNAMIC_DRAW);


    glActiveTexture(GL_TEXTURE0);

    // Video memory
    glGenTextures(1, &lgl_vidram_tex);
    glBindTexture(GL_TEXTURE_BUFFER, lgl_vidram_tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, lgl_vidram_buffer);


    glActiveTexture(GL_TEXTURE1);

    // Palettes
    glGenTextures(1, &lgl_pal_tex);
    glBindTexture(GL_TEXTURE_3D, lgl_pal_tex);
    // you guess it
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // 4 colors per entry, 8 entries per palette, 2 palettes (BG/Obj)
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 4, 8, 2, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, NULL);


    // Vertex line data
    glGenBuffers(1, &lgl_vertex_data);
    glBindBuffer(GL_ARRAY_BUFFER, lgl_vertex_data);

    // 10 horizontal lines, so the vertex shader may find the difference between the lines' ends
    float hori_line[] = { 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f };

    glBufferData(GL_ARRAY_BUFFER, sizeof(hori_line), hori_line, GL_STATIC_DRAW);

    // Vertices
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // OAM data
    glGenBuffers(1, &lgl_vertex_oam);
    glBindBuffer(GL_ARRAY_BUFFER, lgl_vertex_oam);

    // 10 sprites with 2 vertices and 4 ints each
    glBufferData(GL_ARRAY_BUFFER, 10 * 2 * 4 * sizeof(int), NULL, GL_DYNAMIC_DRAW);

    // OAM data comes in here
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 4, GL_INT, 0, (void *)0);


    if (gbc_mode)
        lgl_bg_prg = create_program("shaders/cgb/bg-vertex.glsl", "shaders/cgb/bg-fragment.glsl");
    else
        lgl_bg_prg = create_program("shaders/dmg/bg-vertex.glsl", "shaders/dmg/bg-fragment.glsl");

    glUseProgram(lgl_bg_prg);
    glUniform1i(glGetUniformLocation(lgl_bg_prg, "vidram"), 0);
    glUniform1i(glGetUniformLocation(lgl_bg_prg, "palettes"), 1);

    lgl_y = glGetUniformLocation(lgl_bg_prg, "y");

    lgl_scx = glGetUniformLocation(lgl_bg_prg, "scx");
    lgl_scy = glGetUniformLocation(lgl_bg_prg, "scy");
    lgl_wnd = glGetUniformLocation(lgl_bg_prg, "wnd");
    lgl_lcdc = glGetUniformLocation(lgl_bg_prg, "lcdc");


    if (gbc_mode)
        lgl_obj_prg = create_program("shaders/cgb/obj-vertex.glsl", "shaders/cgb/obj-fragment.glsl");
    else
        lgl_obj_prg = create_program("shaders/dmg/obj-vertex.glsl", "shaders/dmg/obj-fragment.glsl");

    glUseProgram(lgl_obj_prg);
    glUniform1i(glGetUniformLocation(lgl_obj_prg, "vidram"), 0);
    glUniform1i(glGetUniformLocation(lgl_obj_prg, "palettes"), 1);

    lgl_obj_lcdc = glGetUniformLocation(lgl_obj_prg, "lcdc");
    lgl_obj_y = glGetUniformLocation(lgl_obj_prg, "y");
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
                        new_keystate |= VK_A;
                        break;
                    case SDLK_b:
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
                        new_keystate &= ~VK_A;
                        break;
                    case SDLK_b:
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
                    case SDLK_SPACE:
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

    line *= lgl_scr_width * lgl_multiplier;

    SDL_LockSurface(screen);

    for (int x = 0; x < lgl_scr_width / lgl_multiplier; x++)
    {
        int px = (x + offx) & 0xFF;
        uint32_t col = ((uint32_t *)vidmem)[py + px];
        if (lgl_multiplier == 1)
            ((uint32_t *)screen->pixels)[line + x] = SDL_MapRGB(screen->format, (col & 0xFF0000) >> 16, (col & 0xFF00) >> 8, col & 0xFF);
        else
        {
            for (int ax = 0; ax < lgl_multiplier; ax++)
                for (int aline = 0; aline < lgl_multiplier; aline++)
                    ((uint32_t *)screen->pixels)[line + aline * lgl_scr_width + x * lgl_multiplier + ax] = SDL_MapRGB(screen->format, (col & 0xFF0000) >> 16, (col & 0xFF00) >> 8, col & 0xFF);
        }
    }

    SDL_UnlockSurface(screen);

    if (rline == lgl_scr_height / lgl_multiplier - 1)
        SDL_UpdateRect(screen, 0, 0, 0, 0);
}
