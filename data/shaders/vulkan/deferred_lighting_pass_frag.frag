#version 460

#extension GL_EXT_control_flow_attributes : enable

#include "headers/utils.glsl"
#include "headers/tonemapping.glsl"
#include "headers/data.glsl"
#include "headers/brdf.glsl"
#include "headers/lights.glsl"
#include "headers/ibl_utils.glsl"
#include "headers/shadow_mapping.glsl"
#include "headers/volumetric_fog.glsl"

layout(location = 0) out vec4 out_color; // renders to light accumulation buffer
layout(location = 0) flat in int light_instance_index;

layout(set = 0, binding = 0) uniform FrameDataBlock
{
    FrameData data;
} frame;

layout(set = 1, binding = 0) uniform sampler2D gbuffer_base_color;
layout(set = 1, binding = 1) uniform sampler2D gbuffer_normal_vs;
layout(set = 1, binding = 2) uniform sampler2D gbuffer_metalness_roughness;
layout(set = 1, binding = 3) uniform sampler2D gbuffer_depth;

/* Image Based Lighting */
layout(set = 1, binding = 4) uniform sampler2D prefiltered_env_map_diffuse;
layout(set = 1, binding = 5) uniform sampler2D prefiltered_env_map_specular;
layout(set = 1, binding = 6) uniform sampler2D ibl_brdf_integration_map;
layout(set = 1, binding = 7) uniform sampler2D volumetric_lighting;

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

layout (push_constant) uniform LightVolumePassDataBlock
{
    layout(offset = 64)
    float inv_screen_size;
    int light_volume_type;
} ps;

#define LIGHT_VOLUME_DIRECTIONAL 1
#define LIGHT_VOLUME_POINT 2
#define LIGHT_VOLUME_SPOT 3

vec3 decode_normal(vec2 enc)
{
    vec3 n;
    n.xy = enc * 2 - 1;
    n.z = sqrt(1 - dot(n.xy, n.xy));
    return n;
}

void main()
{
    vec2 fragcoord = gl_FragCoord.xy * ps.inv_screen_size;

    BRDFData brdf_data;
    brdf_data.albedo = texture(gbuffer_base_color, fragcoord).rgb;
    brdf_data.metalness_roughness = texture(gbuffer_metalness_roughness, fragcoord).rg;
    brdf_data.normal_ws = vec3(inverse(frame.data.view) * vec4(decode_normal(texture(gbuffer_normal_vs, fragcoord).xy), 0));
    float depth = texture(gbuffer_depth, fragcoord).r;
    vec3 position_ws = vec4( ws_pos_from_depth(fragcoord, depth, frame.data.inv_view_proj), 1).xyz;
    vec3 position_vs = (frame.data.view * vec4(position_ws, 1.0f)).xyz;

    brdf_data.viewdir_ws = normalize(frame.data.eye_pos_ws.xyz-position_ws);
    brdf_data.lightdir_ws = normalize(-lights.dir_light.dir.xyz);
    brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);
    float metallic  = brdf_data.metalness_roughness.x;
    float roughness = brdf_data.metalness_roughness.y;

    out_color = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    vec3 sun_color = lights.dir_light.color.rgb;

    /*
        ----------------------------------------------------------------------------------------------------
        Cascaded Shadow Mapping
        ----------------------------------------------------------------------------------------------------
    */
    int cascade_index = 0;
    mat4 shadow_view_proj = get_cascade_view_proj(position_vs.z, shadow_cascades.data, cascade_index);
    vec4 position_light_space = shadow_view_proj * vec4(position_ws, 1.0f);
    float shadow_factor = get_shadow_factor(tex_shadow_maps, position_light_space, cascade_index);

    if(shadow_cascades.data.show_debug_view)    
    {
        switch(cascade_index) 
        {
            case 0 : 
                out_color.rgb += vec3(1.0f, 0.25f, 0.25f);
                break;
            case 1 : 
                out_color.rgb += vec3(0.25f, 1.0f, 0.25f);
                break;
            case 2 : 
                out_color.rgb += vec3(0.25f, 0.25f, 1.0f);
                break;
            case 3 : 
                out_color.rgb += vec3(1.0f, 1.0f, 0.25f);
                break;
        }
    }

    /* Light volumes */ 
    if (ps.light_volume_type == LIGHT_VOLUME_DIRECTIONAL)
    {
        /*
            ----------------------------------------------------------------------------------------------------
            Direct Lighting
            ----------------------------------------------------------------------------------------------------
        */
        out_color.rgb += brdf_cook_torrance(brdf_data, sun_color) * shadow_factor;

        // vec3 fog = raymarch_fog_sunlight(tex_shadow_maps, cascade_index, frame.data.eye_pos_ws.xyz, position_ws, shadow_view_proj, brdf_data.lightdir_ws, sun_color, frame.data.time);

        vec3 fog = texture(volumetric_lighting, fragcoord).rgb;

        out_color.rgb += fog;
        
        /*
            ----------------------------------------------------------------------------------------------------
            Image Based Lighting
            ----------------------------------------------------------------------------------------------------
        */
        /* Diffuse */
        vec3 diffuse_reflectance = brdf_data.albedo * (1.0 - metallic);
        vec2 diffuse_sample_uv = SampleSphericalMap_ZXY(brdf_data.normal_ws);
        out_color.rgb += diffuse_reflectance * texture(prefiltered_env_map_diffuse, diffuse_sample_uv).rgb;
        
        /* Specular */
        vec3 R = reflect(-brdf_data.viewdir_ws, brdf_data.normal_ws);
        vec2 specular_uv = SampleSphericalMap_ZXY(normalize(R));
        float NoV = clamp(dot(brdf_data.normal_ws, brdf_data.viewdir_ws), 0.0f, 1.0f);
        vec3 T1 = textureLod(prefiltered_env_map_specular, specular_uv , roughness * 6).rgb;
        vec2 brdf = texture(ibl_brdf_integration_map, vec2(NoV, 1-roughness)).xy;
        vec3 F0 = mix(vec3(0.04), brdf_data.albedo, metallic);
        vec3 T2 = (F0 * brdf.x + brdf.y);
        out_color.rgb += T1 * T2;
    }
    else if (ps.light_volume_type == LIGHT_VOLUME_POINT)
    {
        float radius = lights.point_lights[light_instance_index].radius;
        vec3 light_pos = lights.point_lights[light_instance_index].position.xyz;
        vec3 light_color = lights.point_lights[light_instance_index].color;
        vec3 L = light_pos - position_ws;
        float dist = length(L);
        L = normalize(L);
        float atten = atten_sphere_volume(dist, radius);
        brdf_data.lightdir_ws = L;
        brdf_data.halfvec_ws = normalize(brdf_data.lightdir_ws + brdf_data.viewdir_ws);

        vec3 cam_forward = normalize(vec3(frame.data.view[0].w, frame.data.view[1].w, frame.data.view[2].w));

        out_color.rgb += raymarch_fog_omni_spot_light(frame.data.eye_pos_ws.xyz, position_ws, light_pos, light_color, radius, depth, cam_forward);
        out_color.rgb += brdf_cook_torrance(brdf_data,  light_color * 5) * atten;
    }

    out_color = vec4(  out_color.rgb, 1.0);
}