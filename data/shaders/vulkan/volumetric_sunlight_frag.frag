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
    layout(offset = 64)
    float inv_screen_size;
} ps;

/* Shadow */
layout(set = 3, binding = 0) readonly buffer ShadowCascadesSSBO
{
    CascadesData data;
} shadow_cascades;
layout(set = 3, binding = 1) uniform sampler2DArray tex_shadow_maps;

void main()
{
    out_color = vec4(0,0,0,1);

    vec2 fragcoord = gl_FragCoord.xy * vec2(ps.inv_screen_size);
    float depth = texture(z_buffer, fragcoord).r;
    vec3 fragpos_ws = ws_pos_from_depth(fragcoord, depth, frame.data.inv_view_proj);
    vec3 fragpos_vs = vec3(frame.data.view * vec4(fragpos_ws, 1));

    int cascade_index = 0;
    mat4 shadow_view_proj = get_cascade_view_proj(fragpos_vs.z, shadow_cascades.data, cascade_index);

    vec3 fog = raymarch_fog_sunlight(tex_shadow_maps, cascade_index, frame.data.eye_pos_ws.xyz, fragpos_ws, shadow_view_proj, data.dir_light.dir.xyz, data.dir_light.color.xyz);
    out_color.rgb += fog;
}