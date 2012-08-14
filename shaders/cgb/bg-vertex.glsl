#version 330 core

layout (location = 0) in float vertex;

uniform int y;

void main(void)
{
    gl_Position = vec4(2. * vertex - 1., (143. - float(y)) / 72. - 1., 0., 1.);
}
