#version 460

layout(location = 0) out vec4  gbuffer_albedo;
layout(location = 1) out vec4  gbuffer_normalWS;
layout(location = 2) out float gbuffer_depthCS;

layout(location = 0) in vec4 uv;
layout(location = 1) in vec4 normalWS;
layout(location = 2) in vec4 positionCS;
layout(location = 3) in vec4 depthCS;

layout(set = 1, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
} object_data;

layout(set = 2, binding = 0) uniform FrameData 
{ 
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;


void main()
{
    gbuffer_albedo   = vec4(1,0,1,1);//texture(defaultTexture, uv.xy);
    gbuffer_normalWS = vec4(normalWS.xyz, 1.0);
    gbuffer_depthCS  = depthCS.z / depthCS.w;
}