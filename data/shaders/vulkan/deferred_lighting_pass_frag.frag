#version 460

#extension GL_EXT_control_flow_attributes : enable

#include "headers/utils.glsl"
#include "headers/tonemapping.glsl"
#include "headers/framedata.glsl"
#include "headers/brdf.glsl"
#include "headers/lights.glsl"
#include "headers/ibl_utils.glsl"

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

/* Image Based Lighting */
layout(set = 1, binding = 4) uniform sampler2D prefiltered_env_map_diffuse;
layout(set = 1, binding = 5) uniform sampler2D prefiltered_env_map_specular;
layout(set = 1, binding = 6) uniform sampler2D ibl_brdf_integration_map;

vec3 unpack_normal(vec2 n)
{
    float z = sqrt(1 - (n.x*n.x) - (n.y*n.y));
    return vec3(n.x, n.y, z);    
}

void main()
{
    float depth = texture(gbuffer_depth, uv).r;
    vec3 position_ws = ws_pos_from_depth(uv, depth, frame.data.inv_view_proj);
    // Direct lighting
    DirectionalLight dir_light;
    dir_light.L = normalize(-vec3(0, -1, 0.5));
    dir_light.color = vec3(0.8, 0.4, 0.2);

    BRDFData brdf_data;
    brdf_data.albedo = texture(gbuffer_base_color, uv).rgb;
    brdf_data.metalness_roughness = texture(gbuffer_metalness_roughness, uv).rg;
    brdf_data.normal_ws = normalize((texture(gbuffer_normal_ws, uv).xyz));
    brdf_data.viewdir_ws = normalize(frame.data.eye_pos_ws.xyz - position_ws);
    brdf_data.lightdir_ws = dir_light.L;
    brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);
    
    out_color.rgb = vec3(0.0);

    out_color += vec4(brdf_cook_torrance(brdf_data, vec3(0.8, 0.4, 0.3) * 2), 1.0);
    //[[unroll]]
    for(int i = -5; i < 5; i++)
    {
        for(int j = -5; j < 5; j++)
        {
            vec3 l = vec3(i * 2 + cos(frame.data.time), 2.5, j * 2 + sin(frame.data.time))  - position_ws;
            float atten = attenuation_gltf(length(l), 1.0);
            brdf_data.lightdir_ws = normalize(l);
            brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);
            out_color += vec4(brdf_cook_torrance(brdf_data, vec3(1)), 1.0) * atten;
        }
    }

    float metallic  = brdf_data.metalness_roughness.x;
    float roughness = brdf_data.metalness_roughness.y;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, brdf_data.albedo, metallic);

    // IBL Diffuse
    vec3 diffuse_reflectance = brdf_data.albedo * (1.0 - metallic);
    vec2 diffuse_sample_uv = SampleSphericalMap_ZXY(brdf_data.normal_ws);
    out_color.rgb += diffuse_reflectance * texture(prefiltered_env_map_diffuse, diffuse_sample_uv).rgb;

    // IBL Specular
    vec3 R = reflect(-brdf_data.viewdir_ws, brdf_data.normal_ws);
    vec2 specular_uv = SampleSphericalMap_ZXY(R);

    // Split-sum
    float NoV = clamp(dot(brdf_data.normal_ws, brdf_data.viewdir_ws), 0.0, 1.0);
    vec3 T1 = textureLod(prefiltered_env_map_specular, specular_uv , roughness * 6).rgb;
    vec2 brdf = texture(ibl_brdf_integration_map, vec2(NoV, 1-roughness)).xy;
    vec3 T2 = (F0 * brdf.x + brdf.y);
    out_color.rgb += T1 * T2;   

    // Tonemap
    //out_color.rgb = uncharted2_filmic(out_color.rgb);

    out_color = vec4(out_color.rgb, 1.0);
}