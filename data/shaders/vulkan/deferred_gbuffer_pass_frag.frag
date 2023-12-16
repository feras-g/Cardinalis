#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "headers/normal_mapping.glsl" 

layout(location = 1) in vec4 normal_ws;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 vertex_to_eye_ws;

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
    vec4 base_color;
    vec2 metalness_roughness;
} material;

layout(set = 1, binding = 0) uniform sampler2D bindless_tex[];

vec3 unpack_gltf_normal_map(vec3 normal)
{
    // GLTF normal map values are in [0, 1] range.
    return normal * 2.0 - 1.0;
}

void main()
{
    vec3 N = normalize( normal_ws.xyz );
    vec2 metalness_roughness = material.metalness_roughness;
    vec4 base_color  = material.base_color;

    /* Sample base color */
    if(material.texture_base_color_idx != -1)
    {
        base_color = vec4(texture(bindless_tex[material.texture_base_color_idx], uv.xy).rgb, 1.0) * material.base_color;
        if(base_color.a < 0.5) discard;
    }

    gbuffer_base_color = base_color;

    /* Sample normal map
    https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures */
    if(material.texture_normal_map_idx != -1)
    {
		vec3 N_map = unpack_gltf_normal_map(texture(bindless_tex[material.texture_normal_map_idx], uv.xy).xyz); 
		N = perturb_normal(N, N_map, vertex_to_eye_ws, uv.xy);
    }
    gbuffer_normal_ws = vec4(N.xyz, 0); 

    /* https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material */
    /* The textures for metalness and roughness properties are packed together in a single texture called metallicRoughnessTexture. 
        Its green channel contains roughness values and its blue channel contains metalness values. */
    if(material.texture_metalness_roughness_idx != -1)
    {
        metalness_roughness = texture(bindless_tex[material.texture_metalness_roughness_idx], uv).bg * material.metalness_roughness;
    }
    gbuffer_metalness_roughness = metalness_roughness;
}