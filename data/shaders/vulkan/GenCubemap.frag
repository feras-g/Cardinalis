#version 460
#extension GL_EXT_multiview : enable

layout(location=0) in vec4 positionOS;
layout(set = 1, binding = 1) uniform sampler2D equirect_map;

const vec2 inv_atan = vec2(0.1591, 0.3183);

vec2 sample_spherical_map(vec3 v)
{
    vec2 uv = vec2(atan(-v.x, v.z), asin(-v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

layout(location=0) out vec4 out_color;

void main()
{
    vec2 uv = sample_spherical_map(normalize(positionOS.xyz)); // make sure to normalize localPos
    out_color = texture(equirect_map, uv);
}