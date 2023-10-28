#version 460

#include "headers/framedata.glsl"
#include "headers/normal_mapping.glsl"

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
} push_constants;

layout(set = 1, binding = 0) uniform FrameDataBlock 
{ 
    FrameData data;
} frame;

layout(set = 2, binding = 0) uniform sampler2D textures[];

layout(set = 3, binding = 0) buffer ShaderParams { uint a; } shader_params;

void main()
{
    ivec4 mat = ivec4(push_constants.texture_base_color_idx, push_constants.texture_normal_map_idx, push_constants.texture_metalness_roughness_idx, push_constants.texture_emissive_map_idx);

    vec4 L = vec4(cos(frame.data.time), -1, 0, 0);
    
    /* Normal mapping */
    vec4 N_ws = vec4(0);
    if(mat.y > -1)
    {
        vec4 vertex_to_eye = frame.data.camera_pos_ws - position_ws;
        /* Tangent space normals must be mapped to { [-1, 1], [-1, 1], [0, 1] } and normalized 
        *  see https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#additional-textures
        */
        vec3 tangent_space_normal = normalize(texture(textures[mat.y], uv).xyz  * 2.0 - 1.0);
        N_ws.xyz = perturb_normal(normalize(normal_ws).xyz, tangent_space_normal, vertex_to_eye.xyz, uv);
    }

    vec4 base_color = vec4(1,0,1,1);
     
    if (mat.x > -1)
    {
        base_color = texture(textures[mat.x], uv);
    }

    out_color  = base_color * max(0, dot(N_ws, -L));

    if (mat.a > -1)
    {
        out_color += texture(textures[mat.a], uv);
    }

}