#version 460
#define ENABLE_SHADOWS
// #define ENABLE_CUBEMAP_REFLECTIONS

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
layout(set = 0, binding = 5) uniform sampler2DArray  gbuffer_shadow_map;
layout(set = 0, binding = 6) uniform samplerCube cubemap;
layout(set = 0, binding = 7) uniform samplerCube irradiance_map;
layout(std140, set = 0, binding = 8) readonly buffer LightData
{
    DirectionalLight dir_light;
    uint num_point_lights;
    PointLight point_lights[];
} lights;

layout(set = 0, binding = 9) uniform cube_matrices
{
    vec4 splits;
} ubo_cascades;

 layout(set = 0, binding = 10) uniform Matrices
{
    mat4 proj[4];
} CSM_mats;
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
const mat4 bias_matrix = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float sample_shadow_map(vec3 shadow_coord, vec2 offset, uint cascade_index)
{
    float bias = 0.001;
    float shadow = 1.0;
    float shadow_map_depth = texture( gbuffer_shadow_map, vec3(shadow_coord.xy + offset, cascade_index) ).r;
    if( shadow_map_depth < (shadow_coord.z - bias) )
    {
        shadow = 0.027;
    }
    return shadow;
}

const vec2 texmapscale = vec2(1.0/1024.0, 1.0/1024.0);
float eval_shadow_pcf(vec3 shadow_coord, uint cascade_index)
{
    float sum = 0;
    float x,y;
    for(y = -1.5; y <= 1.5; y += 1.0)
    {
        for(x = -1.5; x <= 1.5; x += 1.0)
        {
            vec2 offset = vec2(x, y) * texmapscale;
            sum += sample_shadow_map(shadow_coord, offset, cascade_index);
        }
    }

    return sum / 16.0;
}


vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);

float eval_shadow_poisson(vec3 shadow_coord, uint cascade_index)
{
    float sample_spread = 700.0f + lights.dir_light.color.a;
    float visibility = 1.0;
    float bias = lights.dir_light.direction.a;
    for (int i = 0;i < 4; i++)
    {
        float shadowmap_depth = texture( gbuffer_shadow_map, vec3(shadow_coord.xy + poissonDisk[i] / sample_spread, cascade_index) ).r;

        if ( shadowmap_depth <  shadow_coord.z-bias )
        {
            visibility -= 0.2;
        }
    }

    return visibility;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
    ToggleParams params;
    params.bViewDebugShadow     = false;
    params.bFilterShadowPCF     = true;
    params.bFilterShadowPoisson = false;

    vec4 albedo = texture(gbuffer_color, uv).rgba;
    
    float depthNDC = texture(gbuffer_depth, uv).x;

    vec3 P_WS = ws_pos_from_depth(uv, depthNDC, frame_data.inv_view_proj);
    vec3 P_VS = (frame_data.view * vec4(P_WS, 1.0f)).xyz;

    vec3 cam_pos_ws = frame_data.view_pos.xyz;

    vec3 N_WS = normalize(texture(gbuffer_normal_map, uv).xyz) ; 
    vec3 L = normalize(-lights.dir_light.direction.xyz);
    vec3 V = normalize(cam_pos_ws - P_WS);
    vec3 H = normalize(V+L);

    vec4 metallic_roughness = texture(gbuffer_metallic_roughness, uv);

    float metallic  = metallic_roughness.x *  metallic_roughness.z;
    float roughness = metallic_roughness.y * metallic_roughness.w;

    float light_dist = length(cam_pos_ws - P_WS);
    
    vec3 irradiance = texture(irradiance_map, N_WS).rgb;

    vec3 light_color = lights.dir_light.color.rgb;

#ifdef ENABLE_CUBEMAP_REFLECTIONS
    vec3 R = reflect(-V, N_WS);
    vec3 cubemap_reflection = texture(cubemap, R).rgb * irradiance;
    light_color = clamp(light_color + cubemap_reflection, 0.0, 1.0f);
#endif

float shadow = 1.0f;

vec3 color = BRDF(N_WS, V, L, H, light_color, irradiance, albedo.rgb, metallic, roughness);

#ifdef ENABLE_SHADOWS
	uint cascade_index = 0;
	for(uint i = 0; i < 4 - 1; i++) 
    {
		if(P_VS.z <= ubo_cascades.splits[i]) 
        {	
			cascade_index = i + 1;
		}
	}

    vec4 shadow_coord = bias_matrix * CSM_mats.proj[cascade_index] * vec4(P_WS, 1.0f);
    shadow_coord /= shadow_coord.w;
    shadow_coord = vec4(shadow_coord.x, 1-shadow_coord.y, shadow_coord.z, 1.0);

    if(params.bViewDebugShadow)
    {
	    switch(cascade_index) 
        {
	    	case 0 :
	    		color.rgb *= vec3(1.0f, 0.25f, 0.25f);
	    		break;
	    	case 1 :
	    		color.rgb *= vec3(0.25f, 1.0f, 0.25f);
	    		break;
	    	case 2 :
	    		color.rgb *= vec3(0.25f, 0.25f, 1.0f);
	    		break;
	    	case 3 :
	    		color.rgb *= vec3(1.0f, 1.0f, 0.25f);
	    		break;
	    }
    }
    
    if(params.bFilterShadowPCF)
    {
        shadow = eval_shadow_pcf(shadow_coord.xyz, cascade_index);
    }
    else if(params.bFilterShadowPoisson)
    {
       shadow = eval_shadow_poisson(shadow_coord.xyz, cascade_index); 
    }
    else
    {
        shadow = sample_shadow_map(shadow_coord.xyz, vec2(0, 0), cascade_index);
    }
#endif
    
    color *= shadow;
    out_color = vec4(color, 1.0);
}