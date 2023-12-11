#version 460

#include "headers/vertex.glsl"
#include "headers/framedata.glsl"

layout(location = 0) out vec4 position_ws;
layout(location = 1) out vec4 normal_ws;
layout(location = 2) out vec2 uv;
layout(location = 3) out vec4 instance_color;
layout(location = 4) out vec3 vertex_to_eye_ws;

struct Instance
{
    mat4  model;
    vec4  color;
};

layout(set = 2, binding = 0) readonly buffer VertexBuffer { Vertex data[]; } vtx_buffer;
layout(set = 2, binding = 1) readonly buffer IndexBuffer  { uint   data[]; } idx_buffer;
layout(set = 2, binding = 2) readonly buffer InstanceData 
{ 
    Instance  data[];
} instances;

layout(set = 0, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

layout (push_constant) uniform constants
{
    mat4 model;
} primitive_push_constants;

void main()
{
    uint index = idx_buffer.data[gl_VertexIndex];
    Vertex v = vtx_buffer.data[index];
    vec4 position_os = vec4(v.px, v.py, v.pz, 1.0);

    mat4 model = instances.data[gl_InstanceIndex].model * primitive_push_constants.model;
    instance_color = instances.data[gl_InstanceIndex].color;
    position_ws = model * position_os;
    vec4 position_cs = frame.data.view_proj * position_ws;
    mat4 normal_mat = transpose(inverse(  model  ));

    normal_ws = normal_mat * vec4(v.nx, v.ny, v.nz, 0.0);

    vec2 depth_cs = position_cs.zw;

    uv = vec2(v.u, v.v);

    vertex_to_eye_ws = normalize(frame.data.eye_pos_ws - position_ws).xyz;

    gl_Position = position_cs;
}