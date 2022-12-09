//
#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(binding = 3) uniform sampler2D texFont;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = color * texture(texFont, uv);
}
