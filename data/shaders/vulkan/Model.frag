#version 460

layout(location = 0) out vec4 gbuffer_color;
layout(location = 1) out vec4 gbuffer_normal;
layout(location = 2) out vec4 gbuffer_depth;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;

layout(binding = 3) uniform sampler2D defaultTexture;

void main()
{
    gbuffer_color  = texture(defaultTexture, uv);
    gbuffer_normal = vec4(normal, 1.0f);
    
}