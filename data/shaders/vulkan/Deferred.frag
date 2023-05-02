#version 460
#include "Headers/LightDefinitions.glsl"
#include "Headers/Maths.glsl"

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

void main()
{
    float d = texture(gbuffer_depth, uv).x;
    vec3 posWS = ws_pos_from_depth(uv, d, frame_data.inv_view_proj);
    vec3 normalWS = normalize(texture(gbuffer_WS_normal, uv).xyz);
    vec3 albedo = texture(gbuffer_color, uv).xyz;
     
    vec3 ambient = vec3(0.27);
    vec3 color = albedo * ambient;

    vec3 L = lights.dir_light.direction.xyz;
    vec3 V = normalize(posWS - frame_data.view_pos.xyz);
    float dist = length(L);

    /* Lambert diffuse */
    L = normalize(L);
    float cos_theta = max(dot(normalWS, L), 0.0);
    vec3 diffuse = lights.dir_light.color.rgb * albedo * cos_theta;
    color += diffuse;

//    for(int i=0; i < NUM_LIGHTS; i++)
//    {
//        vec3 L = lights.point_light[i].position.xyz - posWS;
//        vec3 V = normalize(posWS - frame_data.view_pos.xyz);
//        float dist = length(L);
//        float atten = lights.point_light[i].props.x / (pow(dist, 2.0) + 1.0);
//    
//        /* Lambertian diffuse */
//        L = normalize(L);
//        float cos_theta = max(dot(normalWS, L), 0.0);
//        vec3 diffuse = lights.point_light[i].color.rgb * albedo * cos_theta * atten; 
//    
//        color += diffuse;
//    }

    vec4 pos_LS = lights.dir_light.view_proj * vec4(posWS, 1);
    pos_LS = (pos_LS / pos_LS.w);

    vec2 uv_shadow = pos_LS.xy * 0.5 + 0.5;
    float zfrag = pos_LS.z * 0.5 + 0.5;

    float depth = texture(gbuffer_shadow_map, uv_shadow).r;
    float bias = 0.0001;
    if((depth ) < zfrag) // occluded
    {
        color *= diffuse * 0.0;
    }
    else
    {
        color += diffuse;
    }
    
    out_color = vec4(color, 1);
}