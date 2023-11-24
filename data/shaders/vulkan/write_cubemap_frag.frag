#version 460

#include "headers/ibl_utils.glsl"

layout(location = 0) in vec4 positions_os;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 1) uniform sampler2D spherical_env_map;


const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(-v.x, -v.z), asin(-v.y));;
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{   
    vec3 v = normalize(positions_os.xyz);
    // vec2 uv = direction_to_spherical_env_map(v);
    vec2 uv = SampleSphericalMap(v);
    out_color = texture(spherical_env_map, uv);
}
