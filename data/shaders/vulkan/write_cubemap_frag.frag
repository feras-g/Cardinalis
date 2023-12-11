#version 460

#include "headers/ibl_utils.glsl"

layout(location = 0) in vec4 positions_os;
layout(location = 1) in vec4 debug_color;
layout(location = 2) in flat int current_face;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 1) uniform sampler2D spherical_env_map;



void main()
{   
    vec2 uv_env = SampleSphericalMap_ZXY(normalize(positions_os.xyz));
    uv_env = 1-uv_env;
    out_color = vec4( texture(spherical_env_map, uv_env).rgb, 1);
}
