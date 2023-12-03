#version 460

#include "headers/ibl_utils.glsl"

layout(location = 0) in vec4 positions_os;
layout(location = 1) in vec4 debug_color;

layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 1) uniform sampler2D spherical_env_map;

void main()
{   
    vec3 v = normalize(positions_os.xyz);
    vec2 uv = direction_to_spherical_env_map(v);
    out_color = vec4(texture(spherical_env_map, uv).rgb, 1.0);
}
