#version 460

layout(location = 0) out vec2 uv;

struct Vertex
{
    float x,y,z; // Important to keep as float, not vec* or vertex pulling won't work
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
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(v.x, v.y, v.z, 1.0f);
}