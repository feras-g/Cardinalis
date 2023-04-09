#version 460

layout(location = 0) out vec4  gbuffer_albedo;
layout(location = 1) out vec4  gbuffer_normalWS;
layout(location = 2) out float gbuffer_depthCS;

layout(location = 0) in vec4 uv;
layout(location = 1) in vec4 normalWS;
layout(location = 2) in vec4 positionCS;
layout(location = 3) in vec4 depthCS;

layout(binding = 3) uniform sampler2D defaultTexture;

void main()
{
    gbuffer_albedo   = texture(defaultTexture, uv.xy);
    gbuffer_normalWS = vec4(normalWS.xyz, 1.0);
    gbuffer_depthCS  = depthCS.z / depthCS.w;
}