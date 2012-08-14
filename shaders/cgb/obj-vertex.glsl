#version 330 core

layout (location = 0) in float vertex;
layout (location = 1) in ivec4 oam;

noperspective out vec2 rel;
flat out int obj_num, obj_flags;

uniform int y;

void main(void)
{
    rel = vec2(float(oam.x), float(y - oam.y));
    obj_num = oam.z;
    obj_flags = oam.w;

    // Z: flags.7 reset: -.5 (depth: .25) (before standard BG)
    //    flags.7   set:  .5 (depth: .75) (behind standard BG)
    // Remember, this is not the final depth value: depth = (Z + 1) / 2
    gl_Position = vec4(vec2(float(oam.x) + vertex * 8.f, float(144 - y)) * vec2(1. / 80., 1. / 72.) - vec2(1., 1.), float(oam.w >> 7) - .5, 1.);
}
