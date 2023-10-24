#version 460

#include "headers/vertex.glsl"

layout(location = 0) out vec4 position_ws;
layout(location = 1) out vec4 normal_ws;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec4 instance_color;

struct Instance
{
    mat4  model;
    vec4  color;
};

layout(set = 0, binding = 0) readonly buffer VertexBuffer { Vertex data[]; } vtx_buffer;
layout(set = 0, binding = 1) readonly buffer IndexBuffer  { uint   data[]; } idx_buffer;
layout(set = 0, binding = 2) readonly buffer InstanceData 
{ 
    Instance  data[];
} instances;
layout(set = 1, binding = 0) uniform FrameData 
{ 
    mat4 view_proj;
    mat4 inv_view_proj;
} frame_data;

layout (push_constant) uniform constants
{
    int texture_base_color_idx;
    int texture_normal_map_idx;
    int texture_metalness_roughness_idx;
    int texture_emissive_map_idx;
    mat4 model;
} push_constants;

void main()
{
    uint index = idx_buffer.data[gl_VertexIndex];
    Vertex v = vtx_buffer.data[index];
    vec4 position_os = vec4(v.px, v.py, v.pz, 1.0);

    mat4 model = instances.data[gl_InstanceIndex].model * push_constants.model;
    instance_color = instances.data[gl_InstanceIndex].color;
    position_ws = model * position_os;
    vec4 position_cs = frame_data.view_proj * position_ws;
    normal_ws        = model * vec4(v.nx, v.ny, v.nz, 0.0);
    vec2 depth_cs    = position_cs.zw;

    uv = vec2(v.u, v.v);

    gl_Position = position_cs;
}