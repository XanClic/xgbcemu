#version 330 core

layout (location = 0) out vec4 color;

noperspective in vec2 rel;
flat in int obj_num, obj_flags;

uniform usamplerBuffer vidram;
uniform sampler3D palettes;

uniform int lcdc;

void main(void)
{
    int x = abs(((obj_flags & 32) >> 5) * 7 - int(gl_FragCoord.x - rel.x));
    int y = abs(((obj_flags & 64) >> 6) * (7 + ((lcdc & 4) << 1)) - int(rel.y));

    int tile_ofs = ((obj_flags & 0x08) << 13) + (obj_num * 16) + (y << 1);

    int row_p1 = int(texelFetch(vidram, tile_ofs    ));
    int row_p2 = int(texelFetch(vidram, tile_ofs + 1));

    int col = (((row_p2 >> (7 - x)) & 1) << 1) | ((row_p1 >> (7 - x)) & 1);

    color = vec4(texelFetch(palettes, ivec3(col, obj_flags & 7, 1), 0).rgb, float(col) / 3.);
}
