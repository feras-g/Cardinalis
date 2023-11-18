#version 460

layout(set = 0, binding = 0) uniform sampler2D spherical_env_map;
layout(set = 0, binding = 1) uniform ParametersBlock
{
    int env_map_width;
    int env_map_height;
    uint samples;       // Number of samples for importance sampling. Default : 256
} params;

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 out_color;

void main()
{
    out_color = texture(spherical_env_map, uv);
}