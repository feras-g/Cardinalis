#version 460

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 WS_normal;
layout(location = 2) out vec3 position;
    
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
    mat4 mvp; 
} ubo;

layout(binding = 1) readonly buffer VBO { Vertex data[]; } vbo;
layout(binding = 2) readonly buffer IBO { uint data[]; } ibo;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    uv = vec2(v.u, v.v);
    WS_normal = (ubo.model * vec4(v.nx, v.ny, v.nz, 0.0)).xyz;
    gl_Position = ubo.mvp * vec4(v.px, v.py, v.pz, 1.0f);
    position = gl_Position.xyz;
}