#version 460

layout(set = 2, binding = 0) uniform samplerCube cubemap;

layout(location=0) in vec4 positionOS;
layout(location=0) out vec4 out_color;
void main()
{
    vec3 normal = normalize(positionOS.xyz);
    vec3 irradiance = vec3(0.0);
    out_color = texture(cubemap, positionOS.xyz);
}