#version 460

layout(location = 0) out vec4 finalColor;

layout(location = 0) in vec2 uv;
layout(binding = 3) uniform sampler2D defaultTexture;

void main()
{
    finalColor = texture(defaultTexture, uv);
}