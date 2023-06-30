#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "Headers/LightDefinitions.glsl" 
#include "Headers/Maths.glsl" 

layout(location = 0) in vec4 uv;
layout(location = 1) in vec4 normalWS;
layout(location = 2) in vec4 positionCS;
layout(location = 3) in vec4 depthCS;
layout(location = 4) in vec4 positionWS;
layout(location = 5) in vec3 vertexToEye;
layout(location = 6) in mat3 TBN;

layout(location = 0) out vec4  gbuffer_base_color;
layout(location = 1) out vec4  gbuffer_normalWS;
layout(location = 2) out vec4  gbuffer_metallic_roughness;
layout(location = 3) out float gbuffer_depthNDC;

layout(set = 1, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
    vec4 bbox_min_WS;
    vec4 bbox_max_WS;
} object_data;

layout(push_constant) uniform Material
{
    mat4 mat;
    uint tex_base_color_id;
	uint tex_metallic_roughness_id;
	uint tex_normal_id;
	uint tex_emissive_id;
	vec4 base_color_factor;
	float metallic_factor;
	float roughness_factor;
   
} material;

/* texture & Samplers */
layout(set = 2, binding = 0) uniform texture2D textures[];
layout(set = 2, binding = 1) uniform sampler smp_clamp_nearest;
layout(set = 2, binding = 2) uniform sampler smp_clamp_linear;
layout(set = 2, binding = 3) uniform sampler smp_repeat_nearest;
layout(set = 2, binding = 4) uniform sampler smp_repeat_linear;

layout(set = 3, binding = 0) uniform FrameData 
{ 
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

vec4 sample_nearest(uint id, vec2 uv)
{
    return texture(sampler2D(textures[id], smp_repeat_nearest), uv.xy);
}

void main()
{
    vec3 N = normalize( normalWS.xyz );

    /* Sample normal map
    https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures */
    if(material.tex_normal_id != 0)
    {
		vec3 N_map = sample_nearest(material.tex_normal_id, uv.xy).xyz * 2.0 - 1.0;
		N = perturb_normal(N, N_map, vertexToEye, uv.xy);
    }
    gbuffer_base_color = vec4(sample_nearest(material.tex_base_color_id, uv.xy).rgba);
    gbuffer_normalWS   = vec4(N, 1.0f);
    gbuffer_depthNDC   = depthCS.z/depthCS.w;
    /* https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material */
    /* Metallic : G, Roughness: B */

    if(material.tex_metallic_roughness_id != 0)
    {
        gbuffer_metallic_roughness = vec4(sample_nearest(material.tex_metallic_roughness_id, uv.xy).bg, material.metallic_factor, material.roughness_factor);
    }
    else
    {
        gbuffer_metallic_roughness = vec4(0, 0, 0, 1);
    }
}