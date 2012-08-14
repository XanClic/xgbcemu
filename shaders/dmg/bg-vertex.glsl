#version 330 core

layout (location = 0) in float vertex;

uniform int y;

void main(void)
{
    // depth is always .5 for DMG mode
    // (if LCDC.0 is not set, this shader will not be invoked; also, there is no tile attribute)
    gl_Position = vec4(2. * vertex - 1., (143. - float(y)) / 72. - 1., 0., 1.);
}
