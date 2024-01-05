#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "headers/normal_mapping.glsl" 

layout(location = 1) in vec4 normal_vs;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vertex_to_eye_ws;

layout(location = 0) out vec4  gbuffer_base_color;
layout(location = 1) out vec2  gbuffer_normal_vs;
layout(location = 2) out vec2  gbuffer_metalness_roughness;
layout(location = 3) out vec4  gbuffer_lighting_accumulation;

layout(push_constant) uniform constants
{
    layout(offset = 64)
    int texture_base_color_idx;
    int texture_normal_map_idx;
    int texture_metalness_roughness_idx;
    int texture_emissive_map_idx;
    vec4 base_color;
    vec3 emissive_factor;
    float pad;
    vec2 metalness_roughness;
} material;

layout(set = 1, binding = 0) uniform sampler2D bindless_tex[];

vec3 decode_gltf_normal_map(vec3 normal)
{
    // GLTF normal map values are in [0, 1] range.
    return normal * 2 - 1;
}

void main()
{
    vec3 N = normalize( normal_vs.xyz );
    vec2 metalness_roughness = material.metalness_roughness;
    vec4 base_color  = material.base_color;
    vec4 emissive_color  = vec4(material.emissive_factor, 1);

    /* Sample base color */
    if(material.texture_base_color_idx != -1)
    {
        base_color = texture(bindless_tex[material.texture_base_color_idx], uv.xy).rgba * material.base_color;
        if(base_color.a <= 0.05) discard;
    }
    gbuffer_base_color = base_color;

    if(material.texture_emissive_map_idx != -1)
    {
        emissive_color = texture(bindless_tex[material.texture_emissive_map_idx], uv.xy) * vec4(material.emissive_factor, 1) * emissive_color;
    }

    gbuffer_lighting_accumulation = emissive_color;

    /* Sample normal map
    https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures */
    if(material.texture_normal_map_idx != -1)
    {
		vec3 N_map = decode_gltf_normal_map(texture(bindless_tex[material.texture_normal_map_idx], uv.xy).xyz); 
		N = perturb_normal(N, N_map, vertex_to_eye_ws, uv.xy);
    }
    gbuffer_normal_vs = vec2(N.xy * 0.5 + 0.5); 

    /* https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material */
    /* The textures for metalness and roughness properties are packed together in a single texture called metallicRoughnessTexture. 
        Its green channel contains roughness values and its blue channel contains metalness values. */
    if(material.texture_metalness_roughness_idx != -1)
    {
        metalness_roughness = texture(bindless_tex[material.texture_metalness_roughness_idx], uv).bg * material.metalness_roughness;
    }
    gbuffer_metalness_roughness = metalness_roughness;
}