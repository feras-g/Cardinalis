#version 460
#define ENABLE_SHADOWS
// #define ENABLE_CUBEMAP_REFLECTIONS

#include "Headers/LightDefinitions.glsl"
#include "Headers/Maths.glsl"
#include "Headers/BRDF.glsl"
#include "Headers/ShadowMapping.glsl"
#include "Headers/Tonemapping.glsl"
#include "Headers/Fog.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D gbuffer_color;
layout(set = 0, binding = 1) uniform sampler2D gbuffer_VS_normal;
layout(set = 0, binding = 2) uniform sampler2D gbuffer_depth;
layout(set = 0, binding = 3) uniform sampler2D gbuffer_normal_map;
layout(set = 0, binding = 4) uniform sampler2D gbuffer_metallic_roughness;
layout(set = 0, binding = 5) uniform sampler2DArray  gbuffer_shadow_map;
layout(set = 0, binding = 6) uniform samplerCube cubemap;
layout(set = 0, binding = 7) uniform samplerCube irradiance_map;

layout(std140, set = 0, binding = 8) readonly buffer LightData
{
    DirectionalLight dir_light;
    uint num_point_lights;
    PointLight point_lights[];
} lights;

layout(set = 0, binding = 9) readonly buffer ShadowCascadesSSBO
{
    CascadeData data;
} shadow_cascades;

layout(set = 0, binding = 11) uniform texture2D textures[];

layout(set = 1, binding = 0) uniform FrameData
{
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

struct ToggleParams
{
    bool bViewDebugShadow;
    bool bFilterShadowPCF;
    bool bFilterShadowPoisson;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    // ToggleParams params;
    // params.bViewDebugShadow     = false;
    // params.bFilterShadowPCF     = false;
    // params.bFilterShadowPoisson = true;

    // vec4 albedo             = texture(gbuffer_color, uv).rgba;
    // vec2 metallic_roughness = texture(gbuffer_metallic_roughness, uv).rg;
    // float depthNDC          = texture(gbuffer_depth, uv).x;
    // vec3 N_WS               = normalize(texture(gbuffer_normal_map, uv).xyz) ; 
    // vec3 irradiance         = texture(irradiance_map, N_WS).rgb;

    // vec3 P_WS = ws_pos_from_depth(uv, depthNDC, frame_data.inv_view_proj);
    // vec3 P_VS = (frame_data.view * vec4(P_WS, 1.0f)).xyz;

    // vec3 cam_pos_ws = frame_data.view_pos.xyz;

    // vec3 L = normalize(-lights.dir_light.direction.xyz);
    // vec3 V = normalize(cam_pos_ws - P_WS);
    // vec3 H = normalize(V+L);

    // float metallic  = metallic_roughness.r;
    // float roughness = metallic_roughness.g;

    // vec3 light_color = lights.dir_light.color.rgb;

    // vec3 color = vec3(0);

    // /* Directional Light */
    // color += BRDF(N_WS, V, L, H, light_color, irradiance, albedo.rgb, metallic, roughness);

    // /* Point Lights */
    // vec3 point_lights_color = vec3(0);
    // for(uint light_idx=0; light_idx < lights.num_point_lights; light_idx++)
    // {   
    //     vec3 L = normalize(lights.point_lights[light_idx].position.xyz - P_WS);
    //     vec3 H = normalize(V+L);
    //     float distance = length(lights.point_lights[light_idx].position.xyz - P_WS);
    //     float attenuation = attenuation_gltf(distance, lights.point_lights[light_idx].radius);
    //     vec3 light_color = lights.point_lights[light_idx].color.rgb;
    //     point_lights_color += BRDF(N_WS, V, L, H, light_color, irradiance, albedo.rgb, metallic, roughness) * attenuation;
    //     // point_lights_color += BlinnPhong(N_WS, V, L, H, light_color, albedo.rgb) * attenuation;
    // }
    // color += point_lights_color;


    // float shadow_factor = GetShadowFactor(P_WS, P_VS.z, shadow_cascades.data.num_cascades.x, shadow_cascades.data.view_proj, shadow_cascades.data.splits, gbuffer_shadow_map);
    // color = mix(color * 0.27, color, shadow_factor);
        
    // //color += fog(color, abs(P_VS.z) , 0.001);
    // color = uncharted2_filmic(color);

    // out_color = vec4(color, 1.0);
}