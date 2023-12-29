#version 460
#extension GL_EXT_multiview : enable

#include "headers/vertex.glsl"
#include "headers/data.glsl"
#include "headers/shadow_mapping.glsl"

layout(set = 0, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 0, binding = 1) readonly buffer IBO { uint data[]; } ibo;
layout(set = 0, binding = 2) readonly buffer InstanceDataBlock 
{ 
    InstanceData  data[];
} instances;

layout(set = 1, binding = 0) readonly buffer ShadowCascadesSSBO
{
    CascadesData data;
} shadow_cascades;

layout(push_constant) uniform PushConstants
{
    mat4 model;
} primitive_push_constants;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 position_os = vec4(v.px, v.py, v.pz, 1.0);
    gl_Position = shadow_cascades.data.dir_light_view_proj[gl_ViewIndex] * instances.data[gl_InstanceIndex].model * primitive_push_constants.model * position_os;
}