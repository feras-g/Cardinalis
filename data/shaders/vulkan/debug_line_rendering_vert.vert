#version 460

layout(location = 0) out vec4 color;

#include "headers/debug_line_rendering.glsl"

layout(set = 1, binding = 0) uniform FrameData 
{ 
    mat4 view_proj;
    mat4 inv_view_proj;
} frame_data;

void main()
{
    color = vertex_buffer.data[gl_VertexIndex].color;
    gl_Position = frame_data.view_proj * vertex_buffer.data[gl_VertexIndex].position;
}