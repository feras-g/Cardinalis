#version 460

layout(binding = 0) uniform sampler2D gbuffer_color;
layout(binding = 1) uniform sampler2D gbuffer_WS_normal;
layout(binding = 2) uniform sampler2D gbuffer_depth;

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 out_color;

struct DirectionalLight
{
	vec3 color;
	vec3 direction;
};

void main()
{
	DirectionalLight d_light;
	d_light.color = vec3(0.99, 0.98, 0.82);
	d_light.direction = normalize(vec3(0, 0.1, 0));
	
	float fr = 1.0 / 3.14;
	vec3 WS_normal = texture(gbuffer_WS_normal, uv).xyz;

	float nDotL = max(dot(WS_normal, d_light.direction), 0.0);
	vec3 base_color = texture(gbuffer_color, uv).rgb;

	vec3 color = fr * nDotL * d_light.color;
    out_color = vec4(color * base_color, 1.0);
}