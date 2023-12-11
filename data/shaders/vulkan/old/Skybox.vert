#version 460

#include "Headers/VertexDefinitions.glsl"

layout(set = 0, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 0, binding = 1) readonly buffer IBO { uint data[]; } ibo;
layout(set = 1, binding = 0) uniform FrameData 
{ 
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

layout(location=0) out vec4 positionOS;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    positionOS = vec4(v.px, v.py, v.pz, 1.0f);
    vec4 positionCS = frame_data.proj * mat4(mat3(frame_data.view)) * positionOS;

    gl_Position = positionCS.xyww;
}