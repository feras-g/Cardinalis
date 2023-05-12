#version 460
#include "Headers/LightDefinitions.glsl"
#include "Headers/Maths.glsl"
#include "Headers/BRDF.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D gbuffer_color;
layout(set = 0, binding = 1) uniform sampler2D gbuffer_WS_normal;
layout(set = 0, binding = 2) uniform sampler2D gbuffer_depth;
layout(set = 0, binding = 3) uniform sampler2D gbuffer_normal_map;
layout(set = 0, binding = 4) uniform sampler2D gbuffer_metallic_roughness;
layout(set = 0, binding = 5) uniform sampler2D gbuffer_shadow_map;
layout(set = 0, binding = 6) uniform FrameData
{
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;
layout(set = 0, binding = 7) uniform LightData
{
    DirectionalLight dir_light;
} lights;

const mat4 bias_matrix = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float get_shadow_factor(vec3 p_ws, mat4 light_view_proj)
{
    vec4 shadow_coord = bias_matrix * light_view_proj * vec4(p_ws, 1.0f);
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
    vec3 p_ws = ws_pos_from_depth(uv, texture(gbuffer_depth, uv).x, frame_data.inv_view_proj);
    vec3 n_ws = (texture(gbuffer_WS_normal, uv).xyz);
    vec3 l = normalize(-lights.dir_light.direction.xyz);
    vec3 v = normalize(p_ws - frame_data.view_pos.xyz);
    vec3 h = normalize(v+l);
    
    vec3 ambient = vec3(0.27);
    vec3 albedo = texture(gbuffer_color, uv).xyz;
    vec4 metallic_roughness = texture(gbuffer_metallic_roughness, uv);

    float metallic  = metallic_roughness.x *  metallic_roughness.z;
    float roughness = metallic_roughness.y * metallic_roughness.w;
    
    float shadow = get_shadow_factor(p_ws, lights.dir_light.view_proj);

    vec3 color = BRDF(n_ws, v, l, h, lights.dir_light.color.rgb, albedo, metallic, roughness, shadow);
    // vec3 color = BRDF_OGL(n_ws, v, l, h, lights.dir_light.color.rgb, albedo, metallic, roughness);

        // float dist = length(l);
//    for(int i=0; i < NUM_LIGHTS; i++)
//    {
//        vec3 l = lights.point_light[i].position.xyz - p_ws;
//        vec3 v = normalize(p_ws - frame_data.view_pos.xyz);
//        float dist = length(l);
//        float atten = lights.point_light[i].props.x / (pow(dist, 2.0) + 1.0);
//    
//        /* Lambertian diffuse */
//        l = normalize(l);
//        float cos_theta = max(dot(n_ws, l), 0.0);
//        vec3 diffuse = lights.point_light[i].color.rgb * albedo * cos_theta * atten; 
//    
//        color += diffuse;
//    }

    // p_ws.z *= -1; // why ?
    
    out_color = vec4(0.01 * albedo + color.rgb, 1) ;
}