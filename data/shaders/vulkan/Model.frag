#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "Headers/LightDefinitions.glsl" 

layout(location = 0) in vec4 uv;
layout(location = 1) in vec4 normalWS;
layout(location = 2) in vec4 positionCS;
layout(location = 3) in vec4 depthCS;
layout(location = 4) in vec4 positionWS;

layout(location = 0) out vec4  gbuffer_base_color;
layout(location = 1) out vec4  gbuffer_normalWS;
layout(location = 2) out vec4  gbuffer_metallic_roughness;
layout(location = 3) out float gbuffer_depthCS;

layout(set = 1, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
    vec4 bbox_min_WS;
    vec4 bbox_max_WS;
} object_data;

layout(push_constant) uniform Material
{
    uint tex_base_color_id;
	uint tex_metallic_roughness_id;
	uint tex_normal_id;
	uint tex_emissive_id;
	vec4 base_color_factor;
	float metallic_factor;
	float roughness_factor;
} material;

layout(set = 2, binding = 0) uniform FrameData 
{ 
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;
layout(set = 2, binding = 1) uniform texture2D textures[];

/* Samplers */
layout(set = 2, binding = 2) uniform sampler smp_clamp_nearest;
layout(set = 2, binding = 3) uniform sampler smp_clamp_linear;
layout(set = 2, binding = 4) uniform sampler smp_repeat_nearest;
layout(set = 2, binding = 5) uniform sampler smp_repeat_linear;

void main()
{

    gbuffer_base_color         = texture(sampler2D(textures[material.tex_base_color_id], smp_repeat_nearest), uv.xy) * material.base_color_factor;
    gbuffer_normalWS           = vec4(normalize(normalWS.xyz), 1.0);
    gbuffer_depthCS            = depthCS.z / depthCS.w;
    gbuffer_metallic_roughness = vec4(texture(sampler2D(textures[material.tex_metallic_roughness_id], smp_repeat_nearest), uv.xy).xy, material.metallic_factor, material.roughness_factor);
}