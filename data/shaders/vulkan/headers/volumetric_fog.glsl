#ifndef VOLUMETRIC_FOG_GLSL
#define VOLUMETRIC_FOG_GLSL

#include "shadow_mapping.glsl"
#include "math_constants.glsl"

// From GPU Pro 5, Volumetric Light Effects in Killzone: Shadow Fall, 3.4 Dithered Ray Marching
// Bayer matrix
const mat4 dither_pattern = 
    mat4
    (
        0.0f, 0.5f, 0.125f, 0.625f,
        0.75f, 0.25f, 0.875f, 0.375f,
        0.1875f, 0.6875f, 0.0625f, 0.5625,
        0.9375f, 0.4375f, 0.8125f, 0.3125f
    );

float mie_scattering(float VoL)
{
    float g = 0.1;
    float g_sq = g*g;
    return ((1.0 - g_sq) /   (4 * PI  * pow((1.0 + g_sq - 2.0 * g * VoL), 1.5f)));
}

vec3 raymarch_fog_sunlight(sampler2DArray tex_shadow, int cascade_index, vec3 cam_pos_ws, vec3 fragpos_ws, mat4 shadow_view_proj, vec3 L, vec3 sun_color)
{
    const int num_steps = 25;
    vec3 acc = vec3(0);

    vec3 V = cam_pos_ws - fragpos_ws;
    const float step_size = length(V) / float(num_steps);
    V = normalize(V);
    vec3 curr_pos_ws = fragpos_ws;
    curr_pos_ws += step_size * dither_pattern[int(gl_FragCoord.x) % 4][int(gl_FragCoord.y) % 4];
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


vec3 raymarch_fog_omni_spot_light(vec3 eye_pos_ws, vec3 frag_pos_ws, vec3 sphere_center, vec3 light_color, float sphere_radius)
{
    // Ray from camera position to fragment position in WS
    vec3 V = normalize(frag_pos_ws - eye_pos_ws);
    vec3 L = normalize(eye_pos_ws - sphere_center);

    // Find intersection of Ray with Light volume
    float a = dot(V, V);
    float b = 2.0 * dot(L, V);
    float c = dot(L, L) - (sphere_radius*sphere_radius);

    float delta = (b*b);

    vec3 acc = vec3(0.0);

    if(delta > 0)
    {
        float start = -b - a;
        float end   = -b + a;
        float ray_length = end - start;

        // raymarch inside sphere
        vec3 ray_start = eye_pos_ws + start * V;
        vec3 ray_end   = eye_pos_ws + end * V;
        vec3 ray_dir = normalize(ray_end - ray_start);
        
        const int num_steps = 100;
        float step_size = ray_length / float(num_steps);
        
        for(int i = 0; i < num_steps; i++)
        {
            vec3 light_dir = sphere_center - ray_start;
            float att =  clamp(1.0 - length(light_dir), 0.0, 1.0); att *= att;

            light_dir = normalize(light_dir);
            acc += mie_scattering(dot(V, -light_dir)) * light_color * att;
            ray_start += step_size * ray_dir;
        }

        return acc / num_steps;
    }
    else
    {
        return vec3(1,1,1);
    }
    
    return acc;
}

#endif // VOLUMETRIC_FOG_GLSL