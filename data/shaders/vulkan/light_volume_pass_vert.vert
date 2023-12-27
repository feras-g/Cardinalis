#version 460

#include "headers/vertex.glsl"
#include "headers/data.glsl"

layout(location = 0) out int light_instance_index;

layout(set = 2, binding = 0) readonly buffer VertexBufferBlock  { Vertex data[]; } vtx_buffer;
layout(set = 2, binding = 1) readonly buffer IndexBufferBlock   { uint   data[]; } idx_buffer;
layout(set = 2, binding = 2) readonly buffer InstanceDataBlock 
{ 
    InstanceData  data[];
} instances;

layout (push_constant) uniform LightVolumePassDataBlock
{
    mat4 vp;
    float inv_screen_size;
    int light_volume_type;
} ps;

void main()
{
    uint index = idx_buffer.data[gl_VertexIndex];
    Vertex v = vtx_buffer.data[index];
    light_instance_index = gl_InstanceIndex;
    gl_Position = ps.vp * instances.data[gl_InstanceIndex].model * vec4(v.px, v.py, v.pz, 1.0);
}