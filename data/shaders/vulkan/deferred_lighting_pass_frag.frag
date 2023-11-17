#version 460

#include "headers/utils.glsl"
#include "headers/tonemapping.glsl"
#include "headers/framedata.glsl"
#include "headers/brdf.glsl"
#include "headers/lights.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform FrameDataBlock
{
    FrameData data;
} frame;

layout(set = 1, binding = 0) uniform sampler2D gbuffer_base_color;
layout(set = 1, binding = 1) uniform sampler2D gbuffer_normal_ws;
layout(set = 1, binding = 2) uniform sampler2D gbuffer_metalness_roughness;
layout(set = 1, binding = 3) uniform sampler2D gbuffer_depth;

void main()
{
    float depth = texture(gbuffer_depth, uv).r;
    vec3 position_ws = ws_pos_from_depth(uv, depth, frame.data.inv_view_proj);

    BRDFData brdf_data;
    brdf_data.albedo = texture(gbuffer_base_color, uv).rgb;
    brdf_data.metalness_roughness = texture(gbuffer_metalness_roughness, uv).rg;
    brdf_data.normal_ws = normalize(texture(gbuffer_normal_ws, uv).xyz * 2 - 1);
    brdf_data.viewdir_ws = normalize(frame.data.eye_pos_ws.xyz - position_ws);
    brdf_data.lightdir_ws = normalize(-vec3(0, -1, 0));
    brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);

    // out_color = vec4(0,0,0,1);
    // for(int i = -10; i < 10; i++)
    // {
    //     for(int j = -10; j < 10; j++)
    //     {
    //         vec3 l = vec3(i, 1, j)  - position_ws;
    //         float atten = attenuation_gltf(length(l), 1.0);
    //         brdf_data.lightdir_ws = normalize(l);
    //         brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);
    //         out_color += vec4(brdf_cook_torrance(brdf_data, vec3(1.0)), 1.0) * atten;
    //     }
    // }


    out_color = vec4(brdf_cook_torrance(brdf_data, vec3(1.0)), 1.0);

    float roughness = brdf_data.metalness_roughness.g;
    float specular_power = (2 / (roughness*roughness*roughness*roughness)) - 2;
    //out_color = vec4(brdf_blinn_phong(brdf_data, specular_power), 1.0);

    // out_color = vec4(brdf_data.normal_ws, 1);
}