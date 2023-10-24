#version 460
#include "Headers/Tonemapping.glsl"

layout(set = 2, binding = 0) uniform samplerCube cubemap;

layout(location=0) in vec4 positionOS;
layout(location=0) out vec4 out_color;

void main()
{
    vec3 normal = normalize(positionOS.xyz);
    vec3 irradiance = vec3(0.0);
    vec4 color = texture(cubemap, positionOS.xyz);

    out_color = vec4(uncharted2_filmic(color.rgb), 1.0);
}