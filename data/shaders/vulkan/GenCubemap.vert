#version 460
#extension GL_EXT_multiview : enable

#include "Headers/VertexDefinitions.glsl"

layout(set = 0, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 0, binding = 1) readonly buffer IBO { uint data[]; } ibo;
layout(set = 1, binding = 0) uniform Matrices
{
    mat4 view[6];
    mat4 cube_model;
    mat4 proj;
} mat;

layout(location=0) out vec4 positionOS;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    positionOS = vec4(v.px, v.py, v.pz, 1.0f);
    gl_Position = mat.proj * mat.view[gl_ViewIndex] * mat.cube_model * positionOS;
}