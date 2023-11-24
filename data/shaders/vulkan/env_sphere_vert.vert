#version 460

#include "headers/vertex.glsl"
#include "headers/framedata.glsl"
#include "headers/ibl_utils.glsl"

layout(location=0) out vec2 uv;

layout(set = 0, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

layout(set = 1, binding = 0) readonly buffer VertexBuffer { Vertex data[]; } vtx_buffer;
layout(set = 1, binding = 1) readonly buffer IndexBuffer  { uint   data[]; } idx_buffer;

void main()
{
    uint index = idx_buffer.data[gl_VertexIndex];
    Vertex v = vtx_buffer.data[index];
    vec4 position_os = vec4(v.px, v.py, v.pz, 1.0);
    vec4 position_cs = frame.data.proj * mat4(mat3(frame.data.view)) * position_os;

    vec4 n = normalize(vec4(v.nx, v.ny, v.nz, 0.0));
    
    vec2 uv_sphere = vec2(asin(n.x)/PI + 0.5, asin(n.y)/PI + 0.5);

    uv = uv_sphere;
    gl_Position = position_cs.xyww;
}