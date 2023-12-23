#ifndef FOG_GLSL
#define FOG_GLSL

#include "shadow_mapping.glsl"
#include "math_constants.glsl"

float mie_scattering(float VoL)
{
    float g = 0.1;
    float g_sq = g*g;
    return ((1.0 - g_sq) /   (4 * PI  * pow((1.0 + g_sq - 2.0 * g * VoL), 1.5f)));
}

vec3 raymarch_fog(sampler2DArray tex_shadow, int cascade_index, vec3 view_pos_ws, vec3 fragpos_ws, mat4 shadow_view_proj, vec3 L, vec3 sun_color)
{
    const int num_steps = 100;
    vec3 acc = vec3(0);

    vec3 V = view_pos_ws - fragpos_ws;
    const float step_size = length(V) / num_steps;
    V = normalize(V);
    vec3 curr_pos_ws = fragpos_ws;
    for(int i = 0; i < num_steps; i++)
    {
        vec4 curr_pos_light_space = shadow_view_proj * vec4(curr_pos_ws, 1);
        float vis = lookup_shadow(tex_shadow, curr_pos_light_space, vec2(0), cascade_index);

        /* only accumulate fog for fragments that are not in shadow */
        if(vis >= 1)
        {
            acc += mie_scattering(dot(V, -L)) * sun_color;
        }

        curr_pos_ws += step_size * V;
    }

    return acc / num_steps;
}

#endif // FOG_GLSL