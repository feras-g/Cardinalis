#version 460

#extension GL_EXT_control_flow_attributes : enable

#include "headers/utils.glsl"
#include "headers/tonemapping.glsl"
#include "headers/data.glsl"
#include "headers/brdf.glsl"
#include "headers/lights.glsl"
#include "headers/ibl_utils.glsl"
#include "headers/shadow_mapping.glsl"
#include "headers/fog.glsl"

layout(location = 0) in vec4 light_pos_ws;
layout(location = 2) in vec2 uv;
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

/* Direct Lighting */
layout(set = 3, binding = 0) readonly buffer DirectLightingDataBlock
{
    DirectionalLight dir_light;
    PointLight point_lights[];
} lights;

/* Shadow mapping */
layout(set = 4, binding = 0) readonly buffer ShadowCascadesSSBO
{
    CascadesData data;
} shadow_cascades;
layout(set = 4, binding = 1) uniform sampler2DArray tex_shadow_maps;

void main()
{
    vec2 fragcoord = gl_FragCoord.xy * vec2(1.0 / 2048.0);
    float depth = texture(gbuffer_depth, fragcoord).r;
    vec3 position_ws = ws_pos_from_depth(fragcoord, depth, frame.data.inv_view_proj);
    vec3 position_vs = (frame.data.view * vec4(position_ws, 1.0f)).xyz;
    
    BRDFData brdf_data;
    brdf_data.albedo = texture(gbuffer_base_color, fragcoord).rgb;
    brdf_data.metalness_roughness = texture(gbuffer_metalness_roughness, fragcoord).rg;
    brdf_data.normal_ws = normalize((texture(gbuffer_normal_ws, fragcoord).xyz));
    brdf_data.viewdir_ws = normalize(frame.data.eye_pos_ws.xyz - position_ws);
    brdf_data.lightdir_ws = normalize(-lights.dir_light.dir.xyz);
    brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);
    float metallic  = brdf_data.metalness_roughness.x;
    float roughness = brdf_data.metalness_roughness.y;

    out_color = vec4(0,0,0,1);

    // vec3 sun_color =  lights.dir_light.color.rgb;

    // /*
    //     ----------------------------------------------------------------------------------------------------
    //     Cascaded Shadow Mapping
    //     ----------------------------------------------------------------------------------------------------
    // */
    // int cascade_index = 0;
    // mat4 shadow_view_proj = get_cascade_view_proj(position_vs.z, shadow_cascades.data, cascade_index);
    // vec4 position_light_space = shadow_view_proj * vec4(position_ws, 1);
    // float shadow_factor = get_shadow_factor(tex_shadow_maps, position_light_space, cascade_index);

    // if(shadow_cascades.data.show_debug_view)    
    // {
    //     switch(cascade_index) 
    //     {
    //         case 0 : 
    //             out_color.rgb += vec3(1.0f, 0.25f, 0.25f);
    //             break;
    //         case 1 : 
    //             out_color.rgb += vec3(0.25f, 1.0f, 0.25f);
    //             break;
    //         case 2 : 
    //             out_color.rgb += vec3(0.25f, 0.25f, 1.0f);
    //             break;
    //         case 3 : 
    //             out_color.rgb += vec3(1.0f, 1.0f, 0.25f);
    //             break;
    //     }
    // }

    // /*
    //     ----------------------------------------------------------------------------------------------------
    //     Volumetric fog
    //     ----------------------------------------------------------------------------------------------------
    // */ 
    // vec3 fog = raymarch_fog(tex_shadow_maps, cascade_index, frame.data.eye_pos_ws.xyz, position_ws, shadow_view_proj, brdf_data.lightdir_ws, sun_color);
    // out_color.rgb += fog;

    // /*
    //     ----------------------------------------------------------------------------------------------------
    //     Direct Lighting
    //     ----------------------------------------------------------------------------------------------------
    // */

    // /* Directional Light */
    // out_color += vec4(brdf_cook_torrance(brdf_data,  sun_color * 10), 1.0)  * shadow_factor;



    // /*
    //     ----------------------------------------------------------------------------------------------------
    //     Image Based Lighting
    //     ----------------------------------------------------------------------------------------------------
    // */

    // /* Diffuse */
    // vec3 diffuse_reflectance = brdf_data.albedo * (1.0 - metallic);
    // vec2 diffuse_sample_uv = SampleSphericalMap_ZXY(brdf_data.normal_ws);
    // out_color.rgb += diffuse_reflectance * texture(prefiltered_env_map_diffuse, diffuse_sample_uv).rgb;
    
    // /* Specular */
    // vec3 R = reflect(-brdf_data.viewdir_ws, brdf_data.normal_ws);
    // vec2 specular_uv = SampleSphericalMap_ZXY(normalize(R));
    // float NoV = clamp(dot(brdf_data.normal_ws, brdf_data.viewdir_ws), 0.0f, 1.0f);
    // vec3 T1 = textureLod(prefiltered_env_map_specular, specular_uv , roughness * 6).rgb;
    // vec2 brdf = texture(ibl_brdf_integration_map, vec2(NoV, 1-roughness)).xy;
    // vec3 F0 = mix(vec3(0.04), brdf_data.albedo, metallic);
    // vec3 T2 = (F0 * brdf.x + brdf.y);
    // out_color.rgb += T1 * T2;

    float dist = length(light_pos_ws.xyz - position_ws);
    vec3 L = light_pos_ws.xyz - position_ws;
    float atten = attenuation_gltf(dist, 1.0);
    brdf_data.lightdir_ws = normalize(L);
    brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);
    out_color.rgb += brdf_cook_torrance(brdf_data, vec3(1)) * atten;
}