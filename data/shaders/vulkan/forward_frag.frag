#version 460

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 position_ws;
layout(location = 1) in vec4 normal_ws;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 instance_color;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform constants
{
    int texture_base_color_idx;
    int texture_normal_map_idx;
    int texture_metalness_roughness_idx;
    int texture_emissive_map_idx;
    mat4 model;
} push_constants;

layout(set = 2, binding = 0) uniform sampler2D textures[];

layout(set = 3, binding = 0) buffer ShaderParams { uint a; } shader_params;

void main()
{
    ivec4 mat = ivec4(push_constants.texture_base_color_idx, push_constants.texture_normal_map_idx, push_constants.texture_metalness_roughness_idx, push_constants.texture_emissive_map_idx);

    vec4 N_ws = normalize(normal_ws);
    vec4 L_ws = normalize(position_ws - vec4(0,1,0,0));

    vec4 base_color = N_ws;
     
    if (mat.x > -1)
    {
        base_color = texture(textures[mat.x], uv);
    }

    if (mat.a > -1)
    {
        base_color += texture(textures[mat.a], uv);
    }

    out_color  = N_ws;
}