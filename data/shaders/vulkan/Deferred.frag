#version 460

layout(location = 0) out vec4 gbuffer_color;
layout(location = 1) out vec4 gbuffer_vs_normal;
layout(location = 2) out float gbuffer_depth;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 view_space_normal;
layout(location = 2) in vec3 position;

layout(binding = 3) uniform sampler2D defaultTexture;

void main()
{
    gbuffer_color  = texture(defaultTexture, uv);
    gbuffer_vs_normal = vec4(view_space_normal, 1.0f);
    gbuffer_depth = position.z;
}