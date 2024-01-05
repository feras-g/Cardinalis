#version 460

#include "headers/volumetric_fog.glsl"
#include "headers/utils.glsl"
#include "headers/lights.glsl"
#include "headers/data.glsl"

layout(location=0) out vec4 out_color;
layout(location = 0) flat in int light_instance_index;

layout(set = 0, binding = 0) uniform FrameDataBlock
{
    FrameData data;
} frame;

layout(set = 1, binding = 0) readonly buffer VolumetricOmniLightDataBlock
{
    PointLight point_lights[];
} data;

layout(set = 1, binding = 1) uniform sampler2D z_buffer;

layout(push_constant) uniform PushConstantBlock
{
    layout(offset = 64)
    float inv_screen_size;
} ps;

void main()
{
    vec2 fragcoord = gl_FragCoord.xy * vec2(ps.inv_screen_size);
    float depth = texture(z_buffer, fragcoord).r;
    vec3 fragpos_ws = ws_pos_from_depth(fragcoord, depth, frame.data.inv_view_proj);

    PointLight point_light = data.point_lights[light_instance_index];

    vec3 fog = raymarch_fog_omni_spot_light(frame.data.eye_pos_ws.xyz, fragpos_ws, point_light.position, point_light.color, point_light.radius);
    out_color.rgb += vec3(1,0,0);
}