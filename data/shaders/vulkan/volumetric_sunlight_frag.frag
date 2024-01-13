#version 460

#include "headers/volumetric_fog.glsl"
#include "headers/utils.glsl"
#include "headers/lights.glsl"
#include "headers/data.glsl"

layout(location=0) out vec4 out_color;

layout(set = 0, binding = 0) uniform FrameDataBlock
{
    FrameData data;
} frame;

layout(set = 1, binding = 0) readonly buffer VolumetricSunlightDataBlock
{
    DirectionalLight dir_light;
} data;
layout(set = 1, binding = 1) uniform sampler2D z_buffer;

layout(push_constant) uniform PushConstantBlock
{
    layout(offset=64)
    //Sunlight_PushConstantsFragment
    float inv_deferred_render_size;
    ScatteringParameters scattering_params; 
} ps;

/* Shadow */
layout(set = 3, binding = 0) readonly buffer ShadowCascadesSSBO
{
    CascadesData data;
} shadow_cascades;
layout(set = 3, binding = 1) uniform sampler2DArray tex_shadow_maps;

vec3 fog_sunlight(sampler2DArray tex_shadow, int cascade_index, vec3 cam_pos_ws, vec3 fragpos_ws, mat4 shadow_view_proj, vec3 L, vec3 sun_color)
{
    // TODO : Move to UBO
    const int num_steps = ps.scattering_params.num_raymarch_steps;
    const float scattering_amount = ps.scattering_params.amount;
    const float g_mie_scattering = ps.scattering_params.g_mie;
    
    vec3 scattering_factor = vec3(0);

    vec3 V = cam_pos_ws - fragpos_ws;
    const float step_size = length(V) / float(num_steps);
    V = normalize(V);
    vec3 curr_pos_ws = fragpos_ws;
    curr_pos_ws += step_size * dither_pattern[int(gl_FragCoord.x) % 4][int(gl_FragCoord.y) % 4];
    for(int i = 0; i < num_steps; i++)
    {
        vec4 curr_pos_light_space = shadow_view_proj * vec4(curr_pos_ws, 1);
        float vis = lookup_shadow(tex_shadow, curr_pos_light_space, vec2(0), cascade_index);

        /* Only accumulate fog for fragments that are not in shadow */
        if(vis >= 1)
        {
            scattering_factor += mie_scattering(dot(V, -L), g_mie_scattering) * scattering_amount * sun_color;
        }

        curr_pos_ws += step_size * V;
    }

    return scattering_factor / num_steps;
}

void main()
{
    out_color = vec4(0,0,0,1);

    vec2 fragcoord = gl_FragCoord.xy * vec2(ps.inv_deferred_render_size);
    float depth = texture(z_buffer, fragcoord).r;
    vec3 fragpos_ws = ws_pos_from_depth(fragcoord, depth, frame.data.inv_view_proj);
    vec3 fragpos_vs = vec3(frame.data.view * vec4(fragpos_ws, 1));

    int cascade_index = 0;
    mat4 shadow_view_proj = get_cascade_view_proj(fragpos_vs.z, shadow_cascades.data, cascade_index);

    vec3 fog = fog_sunlight(tex_shadow_maps, cascade_index, frame.data.eye_pos_ws.xyz, fragpos_ws, shadow_view_proj, -data.dir_light.dir.xyz, data.dir_light.color.xyz);
    out_color.rgb += fog;
}