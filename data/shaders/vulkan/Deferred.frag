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

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( gbuffer_shadow_map, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.27f;
		}
	}
	return shadow;
}

const mat4 bias_matrix = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float get_shadow_factor(vec3 p_ws, mat4 light_view_proj)
{
    vec4 shadow_coord = bias_matrix * light_view_proj * vec4(p_ws, 1.0f);
    float bias = 0.0001;
    float shadow = 1.0;
    if( texture( gbuffer_shadow_map, shadow_coord.xy ).r < shadow_coord.z - bias)
    {
        shadow = 0.0;
    }
    return shadow;
}

void main()
{
    vec3 p_ws = ws_pos_from_depth(uv, texture(gbuffer_depth, uv).x, frame_data.inv_view_proj);
    vec3 n_ws = normalize(texture(gbuffer_WS_normal, uv).xyz);
    vec3 l = normalize(-lights.dir_light.direction.xyz);
    vec3 v = normalize(p_ws - frame_data.view_pos.xyz);
    vec3 h = normalize(v+l);

    float NoV = abs(dot(n_ws, v)) + 1e-5;
    float NoL = clamp(dot(n_ws, l), 0.0, 1.0);
    float NoH = clamp(dot(n_ws, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);
    
    vec3 ambient = vec3(0.27);
    vec3 albedo = texture(gbuffer_color, uv).xyz;
    vec4 metallic_roughness = texture(gbuffer_metallic_roughness, uv);

    float metallic  = metallic_roughness.y;
    float roughness = metallic_roughness.x;
    
    float shadow = get_shadow_factor(p_ws, lights.dir_light.view_proj);

    vec3 color = BRDF(n_ws, v, l, h, lights.dir_light.color.rgb, albedo, metallic, roughness);
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
    
    out_color = vec4(color, 1) ;
}