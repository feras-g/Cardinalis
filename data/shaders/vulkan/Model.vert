#version 460
#include "Headers/LightDefinitions.glsl"

layout(location = 0) out vec4 uv;
layout(location = 1) out vec4 normalWS;
layout(location = 2) out vec4 positionCS;
layout(location = 3) out vec4 depthCS;
layout(location = 4) out vec4 positionWS;
    
struct Vertex
{
    float px,py,pz; // Important to keep as float, not vec* or vertex pulling won't work
    float nx,ny,nz;
    float u, v;
};

layout(set = 0, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 0, binding = 1) readonly buffer IBO { uint data[]; } ibo;

layout(set = 1, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
    vec4 bbox_min_WS;
    vec4 bbox_max_WS;
} object_data;

layout(set = 2, binding = 0) uniform FrameData 
{ 
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);
    vec4 normalOS   = vec4(v.nx, v.ny, v.nz, 0.0);

    uv.xy = vec2(v.u, v.v);
    normalWS   = object_data.model * normalOS;
    positionCS = object_data.mvp * positionOS;
    depthCS.xy = positionCS.zw;
    positionWS = object_data.model * positionOS;
    gl_Position = positionCS;
}