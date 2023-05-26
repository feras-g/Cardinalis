#version 460
#include "Headers/VertexDefinitions.glsl"
#include "Headers/LightDefinitions.glsl"

layout(location=0) out vec2 uv;
layout(set = 0, binding = 0) uniform LightData
{
    DirectionalLight dir_light;
} lights;

layout(set = 1, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 1, binding = 1) readonly buffer IBO { uint data[]; } ibo;

layout(set = 2, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
    vec4 bbox_min_WS;
    vec4 bbox_max_WS;
} object_data;

layout(set = 3, binding = 0) uniform FrameData
{
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

layout(push_constant) uniform PushConstants
{
    mat4 model;
} mat;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);
    gl_Position = lights.dir_light.view_proj * mat.model * positionOS;
}