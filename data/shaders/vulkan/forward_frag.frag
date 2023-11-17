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
    vec3 L_WS = normalize(-vec3(1, -1, 0));
    vec3 N_WS = normalize(normal_ws).xyz;
    vec3 V_WS = normalize(frame.data.eye_pos_ws.xyz - position_ws.xyz);  /* Vertex to eye */
    vec3 H_WS = normalize(L_WS + V_WS);
    
    /* Normal mapping */
    if(mat.y > -1)
    {
        
        /* Tangent space normals must be mapped to { [-1, 1], [-1, 1], [0, 1] } and normalized 
        *  see https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures
        */
        vec3 N_TS = normalize(texture(textures[mat.y], uv).xyz  * 2.0 - 1.0);
        N_WS = perturb_normal(N_WS, N_TS, V_WS, uv);
    }

    vec4 base_color = instance_color;
     
    if (mat.x > -1)
    {
        base_color = texture(textures[mat.x], uv);
    }

    /* Metallic-Roughness */
    float roughness = 1.0f;
    if(mat.z > -1)
    {
        roughness = texture(textures[mat.z], uv).g;
    }

    float specular_power = (2 / (roughness*roughness*roughness*roughness)) - 2; /* Blinn-Phong [2]: https://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html */
    vec3 color_blinn_phong = brdf_blinn_phong(base_color.rgb, light_color, N_WS, L_WS, H_WS, specular_power);

    out_color  = vec4(color_blinn_phong, 1);

    if (mat.a > -1)
    {
        out_color.rgb += texture(textures[mat.a], uv).rgb;
    }
    
}