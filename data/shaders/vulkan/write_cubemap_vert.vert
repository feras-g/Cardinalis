#version 460
#extension GL_EXT_multiview : enable

#include "headers/vertex.glsl"
#include "headers/framedata.glsl"

layout(location = 0) out vec4 position_os;
layout(location = 1) out vec4 debug_color;
layout(location = 2) out int current_face;
layout(location = 3) out vec2 uv;

layout(set = 0, binding = 0) readonly buffer VertexBuffer { Vertex data[]; } vtx_buffer;
layout(set = 0, binding = 1) readonly buffer IndexBuffer  { uint   data[]; } idx_buffer;

layout(set = 1, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

layout(set = 2, binding = 0) uniform Matrices
{
    vec4 colors[6];
    mat4 view[6];
    mat4 cube_model;
    mat4 proj;
} mat;


const vec4 layer_index_color[6] =
{
    vec4(1,0,0,1),
    vec4(0,1,1,1),
    vec4(0,1,0,1),
    vec4(1,0,1,1),
    vec4(0,0,1,1),
    vec4(1,1,0,1),
};


void main()
{
    uint index = idx_buffer.data[gl_VertexIndex];
    Vertex v = vtx_buffer.data[index];
    position_os = vec4(v.px, v.py, v.pz, 1.0);
    debug_color = layer_index_color[gl_ViewIndex];
    vec4 position_ws =  mat.cube_model * position_os;
    uv = vec2(v.u, v.v);
    current_face = gl_ViewIndex;
    gl_Position = mat.proj * mat.view[gl_ViewIndex] * position_ws;
}
