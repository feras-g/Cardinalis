#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "headers/LightDefinitions.glsl" 
#include "headers/normal_mapping.glsl" 

layout(location = 1) in vec4 normal_ws;
layout(location = 2) in vec2 uv;
layout(location = 4) in vec3 vertex_to_eye_ws;

layout(location = 0) out vec4  gbuffer_base_color;
layout(location = 1) out vec4  gbuffer_normal_ws;
layout(location = 2) out vec2  gbuffer_metalness_roughness;

layout(push_constant) uniform constants
{
    layout(offset = 64)
    int texture_base_color_idx;
    int texture_normal_map_idx;
    int texture_metalness_roughness_idx;
    int texture_emissive_map_idx;
} material;

layout(set = 1, binding = 0) uniform sampler2D bindless_tex[];

void main()
{
    vec3 N = normalize( normal_ws.xyz );

    /* Sample base color */
    {
        gbuffer_base_color = vec4(texture(bindless_tex[material.texture_base_color_idx], uv.xy).rgba);
        if(gbuffer_base_color.a < 1) discard;
    }

    /* Sample normal map
    https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures */
    if(material.texture_normal_map_idx != -1)
    {
		vec3 N_map = texture(bindless_tex[material.texture_normal_map_idx], uv.xy).xyz * 2.0 - 1.0;
		N = perturb_normal(N, N_map, vertex_to_eye_ws, uv.xy);
    }
    else
    {
        gbuffer_normal_ws   = vec4(N, 1.0f);
    }

    /* https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material */
    /* Metallic : G, Roughness: B */
    if(material.texture_metalness_roughness_idx != 0)
    {
        gbuffer_metalness_roughness = texture(bindless_tex[material.texture_metalness_roughness_idx], uv.xy).bg;
    }
    else
    {
        gbuffer_metalness_roughness = vec2(0, 1.0);
    }
}