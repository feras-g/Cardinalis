#version 460

#include "headers/framedata.glsl"
#include "headers/normal_mapping.glsl"
#include "headers/brdf.glsl"

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 position_ws;
layout(location = 1) in vec4 normal_ws;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 instance_color;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform constants
{
    layout(offset = 64)
    int texture_base_color_idx;
    int texture_normal_map_idx;
    int texture_metalness_roughness_idx;
    int texture_emissive_map_idx;
    vec4 base_color;
    vec2 metalness_roughness;
} push_constants;

layout(set = 0, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(set = 3, binding = 0) buffer ShaderParams 
{ 
    bool enable_normal_mapping; 
} shader_params;

void main()
{
    ivec4 mat = ivec4(push_constants.texture_base_color_idx, push_constants.texture_normal_map_idx, push_constants.texture_metalness_roughness_idx, push_constants.texture_emissive_map_idx);
    
    vec3 light_color = vec3(0.27,0.1,0.01);

    BRDFData brdf_data;
    brdf_data.albedo = push_constants.base_color.rgb;
    brdf_data.metalness_roughness = push_constants.metalness_roughness;
    brdf_data.normal_ws = normalize(normal_ws).xyz;
    brdf_data.viewdir_ws = normalize(frame.data.eye_pos_ws - position_ws).xyz;
    brdf_data.lightdir_ws = normalize(-vec3(1, -1, 0));
    brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);
    
    /* Normal mapping */
    if(mat.y > -1)
    {
        
        /* Tangent space normals must be mapped to { [-1, 1], [-1, 1], [0, 1] } and normalized 
        *  see https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures
        */
        vec3 N_TS = normalize(texture(textures[mat.y], uv).xyz  * 2.0 - 1.0);
        brdf_data.normal_ws = perturb_normal(brdf_data.normal_ws, N_TS, brdf_data.viewdir_ws, uv);
    }

    brdf_data.albedo = push_constants.base_color.rgb;
     
    if (mat.x > -1)
    {
        brdf_data.albedo = texture(textures[mat.x], uv).rgb;
    }

    /* Metallic-Roughness */
    if(mat.z > -1)
    {
        brdf_data.metalness_roughness = texture(textures[mat.z], uv).rg;
    }

    if (mat.a > -1)
    {
        out_color.rgb += texture(textures[mat.a], uv).rgb;
    }
}