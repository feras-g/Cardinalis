#version 460

#include "headers/vertex.glsl"
#include "headers/data.glsl"
#include "headers/ibl_utils.glsl"

layout(location = 0) out vec4 position_os;

layout(set = 0, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

layout(push_constant) uniform constants
{
    mat4 model;
} ps;


layout(set = 1, binding = 0) readonly buffer VertexBuffer { Vertex data[]; } vtx_buffer;
layout(set = 1, binding = 1) readonly buffer IndexBuffer  { uint   data[]; } idx_buffer;

void main()
{
    uint index = idx_buffer.data[gl_VertexIndex];
    Vertex v = vtx_buffer.data[index];
    position_os = vec4(v.px, v.py, v.pz, 1.0);
    vec4 position_ws = ps.model * position_os;
    vec4 position_cs = frame.data.proj * mat4(mat3(frame.data.view)) *  position_ws;

    gl_Position = position_cs.xyww;
}