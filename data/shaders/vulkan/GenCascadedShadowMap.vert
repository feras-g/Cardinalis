#version 460
#extension GL_EXT_multiview : enable

#include "Headers/VertexDefinitions.glsl"
#include "Headers/LightDefinitions.glsl"

layout(location=0) out vec2 uv;

layout(set = 0, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 0, binding = 1) readonly buffer IBO { uint data[]; } ibo;

layout(set = 1, binding = 0) uniform Matrices
{
    mat4 proj[4];
} csm_mats;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    mat4 dir_light_view;
} mat;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);
    gl_Position = csm_mats.proj[gl_ViewIndex] * mat.model * positionOS;
}