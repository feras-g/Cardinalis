#version 460
#include "Headers/LightDefinitions.glsl"
#include "Headers/Maths.glsl"
#include "Headers/BRDF.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D gbuffer_color;
layout(set = 0, binding = 1) uniform sampler2D gbuffer_VS_normal;
layout(set = 0, binding = 2) uniform sampler2D gbuffer_depth;
layout(set = 0, binding = 3) uniform sampler2D gbuffer_normal_map;
layout(set = 0, binding = 4) uniform sampler2D gbuffer_metallic_roughness;
layout(set = 0, binding = 5) uniform sampler2D gbuffer_shadow_map;
layout(set = 0, binding = 6) uniform samplerCube cubemap;
layout(set = 0, binding = 7) uniform samplerCube irradiance_map;
layout(set = 0, binding = 8) uniform LightData
{
    DirectionalLight dir_light;
} lights;

layout(set = 1, binding = 0) uniform FrameData
{
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

const mat4 bias_matrix = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float get_shadow_factor(vec3 P_WS, mat4 light_view_proj)
{
    vec4 shadow_coord = bias_matrix * light_view_proj * vec4(P_WS, 1.0f);
    float bias = 0.0005;
    float shadow = 1.0;
    if( texture( gbuffer_shadow_map, vec2(shadow_coord.x, 1-shadow_coord.y) ).r < shadow_coord.z - bias)
    {
        shadow = 0.27;
    }
    return shadow;
}

void main()
{
    vec3 P_WS = ws_pos_from_depth(uv, texture(gbuffer_depth, uv).x, frame_data.inv_view_proj);
    vec3 N_WS = texture(gbuffer_VS_normal, uv).xyz;
    vec3 l = normalize(lights.dir_light.direction.xyz);
    vec3 v = normalize(frame_data.view_pos.xyz - P_WS);
    vec3 h = normalize(v+l);
    vec3 R = reflect(-v, N_WS);

    float light_dist = length(frame_data.view_pos.xyz - P_WS);
    
    vec3 irradiance = texture(irradiance_map, N_WS).rgb;
    vec3 sky_reflection = texture(cubemap, R).rgb * irradiance;

    vec3 albedo = texture(gbuffer_color, uv).xyz;
    vec4 metallic_roughness = texture(gbuffer_metallic_roughness, uv);

    float metallic  = metallic_roughness.x *  metallic_roughness.z;
    float roughness = metallic_roughness.y * metallic_roughness.w;
    
    float shadow = get_shadow_factor(P_WS, lights.dir_light.view_proj);

    vec3 color = BRDF(N_WS, v, l, h, lights.dir_light.color.rgb + sky_reflection, irradiance, albedo, metallic, roughness, shadow);
    
    out_color = vec4(color.rgb, 1) ;
}