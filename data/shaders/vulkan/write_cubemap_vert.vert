#version 460
#extension GL_EXT_multiview : enable

#include "headers/vertex.glsl"
#include "headers/framedata.glsl"

layout(location = 0) out vec4 position_os;

layout(set = 0, binding = 0) readonly buffer VertexBuffer { Vertex data[]; } vtx_buffer;
layout(set = 0, binding = 1) readonly buffer IndexBuffer  { uint   data[]; } idx_buffer;

layout(set = 1, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

layout(set = 2, binding = 0) uniform Matrices
{
    mat4 view[6];
    mat4 cube_model;
    mat4 proj;
} mat;

void main()
{
    uint index = idx_buffer.data[gl_VertexIndex];
    Vertex v = vtx_buffer.data[index];
    position_os = vec4(v.px, v.py, v.pz, 1.0);
    gl_Position = mat.proj * mat.view[gl_ViewIndex] * mat.cube_model * position_os;
}
