#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4  gbuffer_base_color;
layout(location = 1) out vec4  gbuffer_normalWS;
layout(location = 2) out vec4  gbuffer_metallic_roughness;
layout(location = 3) out vec4  gbuffer_normal_map;
layout(location = 4) out float gbuffer_depthCS;

layout(location = 0) in vec4 uv;
layout(location = 1) in vec4 normalWS;
layout(location = 2) in vec4 positionCS;
layout(location = 3) in vec4 depthCS;


layout(set = 1, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
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
    gbuffer_base_color     = texture(sampler2D(textures[material.tex_base_color_id], smp_clamp_linear), uv.xy) * material.base_color_factor;
    gbuffer_normalWS    = vec4(normalize(normalWS.xyz), 1.0);
    gbuffer_depthCS    = depthCS.z / depthCS.w;
    gbuffer_normal_map = vec4(texture(sampler2D(textures[material.tex_normal_id], smp_clamp_linear), uv.xy).rg * 2 - 1, 0, 1); // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures
    gbuffer_metallic_roughness = vec4(texture(sampler2D(textures[material.tex_metallic_roughness_id], smp_clamp_linear), uv.xy).yz, material.metallic_factor, material.roughness_factor);
}