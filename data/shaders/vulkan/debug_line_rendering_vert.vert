#version 460

layout(location = 0) out vec4 color;

#include "headers/debug_line_rendering.glsl"
#include "headers/data.glsl"

layout(set = 1, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

void main()
{
    color = vertex_buffer.data[gl_VertexIndex].color;
    gl_Position = frame.data.view_proj * vertex_buffer.data[gl_VertexIndex].position;
}