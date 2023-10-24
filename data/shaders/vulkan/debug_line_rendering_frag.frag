#version 460

#include "headers/debug_line_rendering.glsl"

layout(location = 0) in vec4  in_color;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = in_color;
}