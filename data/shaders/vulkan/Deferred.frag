#version 460

layout(set = 0, binding = 0) uniform sampler2D gbuffer_color;
layout(set = 0, binding = 1) uniform sampler2D gbuffer_WS_normal;
layout(set = 0, binding = 2) uniform sampler2D gbuffer_depth;
layout(set = 0, binding = 3, std140) uniform FrameData
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    mat4 mvp;
    vec4 view_pos;
} ubo;

struct PointLight
{
    vec4 position;
    vec4 color;
    vec4 props;
};

#define NUM_LIGHTS 1024

layout(set = 0, binding = 4) uniform LightData
{
    PointLight point_light[NUM_LIGHTS];
} lights;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

vec3 ws_pos_from_depth(vec2 uv, float z_ndc)
{
    // Compute NDC position
    float x = uv.x * 2 - 1;
	float y = (1-uv.y) * 2 - 1;
    vec4 ndc_pos = vec4(x,y,z_ndc, 1.0);

    // NDC -> View-Space
    // After applying inverse view proj, vector is still in projective space,
    // this is why we divide by W
    vec4 ws_pos = ubo.inv_view_proj * ndc_pos; 
    ws_pos.xyz /= ws_pos.w;

    return ws_pos.xyz;
}

void main()
{
    float d = texture(gbuffer_depth, uv).x;
	vec3 posWS = ws_pos_from_depth(uv, d);
    vec3 normalWS = normalize(texture(gbuffer_WS_normal, uv).xyz);
    vec3 albedo = texture(gbuffer_color, uv).xyz;

    /* Ambiant color */
    vec3 ambient = vec3(0.27);

    vec3 color = albedo * ambient;

    for(int i=0; i < NUM_LIGHTS; i++)
    {
        vec3 L = lights.point_light[i].position.xyz - posWS;
        vec3 V = normalize(posWS - ubo.view_pos.xyz);
        float dist = length(L);
        float atten = lights.point_light[i].props.x / (pow(dist, 2.0) + 1.0);

        /* Lambertian diffuse */
        L = normalize(L);
        float cos_theta = max(dot(normalWS, L), 0.0);
        vec3 diffuse = lights.point_light[i].color.rgb * albedo * cos_theta * atten; 

        color += diffuse;
    }

    out_color = vec4(color, 1.0);
}