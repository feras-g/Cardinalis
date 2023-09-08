#version 460
#include "Headers/LightDefinitions.glsl"
#include "Headers/VertexDefinitions.glsl"

layout(location = 0) out vec2 uv;
layout(location = 5) out vec3 vertexToEye;

layout(set = 0, binding = 0) readonly buffer MeshData
{
    InstanceData instances[];
} mesh_data;

layout(set = 3, binding = 0) uniform FrameData 
{ 
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

void main()
{
    /* Get vertex data */
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);

    mat4 model = mesh_data.instances[gl_InstanceIndex].model;
    uv.xy = vec2(v.u, v.v); 
    normalWS       = normalize(model * vec4(v.nx, v.ny, v.nz, 0.0));

    positionCS =  frame_data.proj * frame_data.view * model * positionOS;
    depthCS.xy = positionCS.zw;
    positionWS = model * positionOS;
    vertexToEye = normalize(frame_data.view_pos - positionWS).xyz;

    gl_Position = positionCS;
}