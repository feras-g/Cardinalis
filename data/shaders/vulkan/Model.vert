#version 460

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 normal;

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
} ubo;

layout(binding = 1) readonly buffer VBO { Vertex data[]; } vbo;
layout(binding = 2) readonly buffer IBO { uint data[]; } ibo;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    uv = vec2(v.u, v.v);
    normal = vec3(v.nx, v.ny, v.nz);
    
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(v.px, v.py, v.pz, 1.0f) + 
    vec4(gl_InstanceIndex * 5, 0, 0, 0);
}