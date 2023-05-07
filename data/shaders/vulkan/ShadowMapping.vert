#version 460
#include "Headers/VertexDefinitions.glsl"
#include "Headers/LightDefinitions.glsl"

layout(location=0) out vec2 uv;
layout(location=1) out vec4 pos_LS;
layout(set = 0, binding = 0) uniform LightData
{
    DirectionalLight dir_light;
} lights;
layout(set = 0, binding = 1) uniform sampler2D gbuffer_depth;
layout(set = 0, binding = 2) uniform FrameData
{
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

layout(set = 1, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 1, binding = 1) readonly buffer IBO { uint data[]; } ibo;

layout(set = 2, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
    vec4 bbox_min_WS;
    vec4 bbox_max_WS;
} object_data;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);
    pos_LS = lights.dir_light.view_proj * object_data.model * positionOS;
    gl_Position = pos_LS;
}