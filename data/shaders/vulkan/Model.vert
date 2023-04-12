#version 460

layout(location = 0) out vec4 uv;
layout(location = 1) out vec4 normalWS;
layout(location = 2) out vec4 positionCS;
layout(location = 3) out vec4 depthCS;

    
struct Vertex
{
    float px,py,pz; // Important to keep as float, not vec* or vertex pulling won't work
    float nx,ny,nz;
    float u, v;
};

layout(binding = 0) uniform UniformData 
{ 
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    mat4 mvp;
    vec4 view_pos;
} ubo;

layout(binding = 1) readonly buffer VBO { Vertex data[]; } vbo;
layout(binding = 2) readonly buffer IBO { uint data[]; } ibo;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);
    vec4 normalOS = vec4(v.nx, v.ny, v.nz, 0.0);

    uv.xy = vec2(v.u, v.v);
    normalWS   = ubo.model * normalOS;
    positionCS = ubo.model * positionOS;
    depthCS.xy = positionCS.zw;

    gl_Position = ubo.mvp * positionOS;
}