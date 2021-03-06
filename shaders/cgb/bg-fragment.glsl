#version 330 core

layout (location = 0) out vec4 color;

uniform usamplerBuffer vidram;
uniform sampler3D palettes;
uniform int y, scx, scy;
uniform ivec2 wnd;

uniform int lcdc;

void main(void)
{
    int x = int(gl_FragCoord.x);

    int tile_map_base, tile_base = (~lcdc & 0x10) << 8; // 0x1000 / 0x0000

    ivec2 wnd_rel = ivec2(x, y) - wnd;

    int bgx, bgy;
    if ((wnd_rel.x < 0) || (wnd_rel.y < 0))
    {
        tile_map_base = 0x1800 + ((lcdc & 0x08) << 7); // 0x1800 / 0x1C00
        bgx = (scx + x) & 0xFF;
        bgy = (scy + y) & 0xFF;
    }
    else
    {
        tile_map_base = 0x1800 + ((lcdc & 0x40) << 4); // 0x1800 / 0x1C00
        bgx = wnd_rel.x;
        bgy = wnd_rel.y;
    }

    int tile_index = (bgx >> 3) | ((bgy & 0xF8) << 2);

    int tile = int(texelFetch(vidram, tile_map_base + tile_index));
    int tile_attr = int(texelFetch(vidram, tile_map_base + 0x2000 + tile_index));

    // interpret as 2's complement as neccessary (lcdc.4 not set, bg_tile.7 set -> negative (subtract 256))
    tile -= ((~lcdc & 0x10) & ((tile & 0x80) >> 3)) << 4;

    //                        bank                 base        number                y offset (mirror included)
    int tile_ofs = ((tile_attr & 0x08) << 10) + tile_base + (tile * 16) + (abs(((tile_attr >> 6) & 1) * 7 - (bgy & 7)) << 1);

    int row_p1 = int(texelFetch(vidram, tile_ofs    ));
    int row_p2 = int(texelFetch(vidram, tile_ofs + 1));

    // x offset in int (mirror included)
    x = abs(((~tile_attr >> 5) & 1) * 7 - (bgx & 7));

    int col = (((row_p2 >> x) & 1) << 1) | ((row_p1 >> x) & 1);

    color = texelFetch(palettes, ivec3(col, tile_attr & 7, 0), 0);

    // LCDC.0 reset: always behind (-> depth = .9)
    // attr.7 set: always in front (-> depth = .0)
    // color 0: always behind (-> depth = .9)
    gl_FragDepth = min(.9, float(~lcdc & 1) + float((~tile_attr & 0x80) >> 7) * .5 + 1. - clamp(float(col), 0., 1.));
}
