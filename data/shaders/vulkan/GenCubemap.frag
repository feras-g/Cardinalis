#version 460
#extension GL_EXT_multiview : enable

layout(location=0) in vec4 positionOS;
layout(set = 1, binding = 1) uniform sampler2D equirect_map;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

layout(location=0) out vec4 out_color;

void main()
{
    vec2 uv = SampleSphericalMap(normalize(positionOS.xyz)); // make sure to normalize localPos
    out_color = texture(equirect_map, uv);
}