#ifndef VOLUMETRIC_FOG_GLSL
#define VOLUMETRIC_FOG_GLSL

#include "shadow_mapping.glsl"
#include "math_constants.glsl"
#include "lights.glsl"


struct ScatteringParameters
{
	float g_mie;				/* Float value between 0 and 1 controlling how much light will scatter in the forward direction. (Henyey-Greenstein phase function) */
	float amount;				/* Float value controlling how much volumetric light is visible. */
	int num_raymarch_steps;
};

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


/*
    g: Controls how much light will scatter in the forward direction. Value between 0 and 1
*/
float mie_scattering(float VoL, float g)
{
    float g_sq = g*g;
    return ((1.0 - g_sq) /   (4 * PI  * pow((1.0 + g_sq - 2.0 * g * VoL), 1.5f)));
}

vec3 raymarch_fog_sunlight(sampler2DArray tex_shadow, int cascade_index, vec3 cam_pos_ws, vec3 fragpos_ws, mat4 shadow_view_proj, vec3 L, vec3 sun_color)
{
    // TODO : Move to UBO
    const int num_steps = 25;
    const float scattering_amount = 2;
    const float g_mie_scattering = 0.7;
    
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

float remap01(float low, float high, float val)
{
    return (val - low) / (high - low);
}

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

vec3 raymarch_fog_omni_spot_light(vec3 ws_cam_pos, vec3 ws_frag_pos, vec3 sphere_center, vec3 light_color, float sphere_radius, float depth, vec3 cam_forward)
{
    vec3 center = sphere_center;
    float radius = sphere_radius * 10;

    vec3 ray_orig = ws_cam_pos;
    vec3 ray_dir = ws_frag_pos - ray_orig;
    float ray_length = length(ray_dir);
    ray_dir = normalize(ray_dir);
    vec3 light_to_cam = normalize(ws_cam_pos - center);


	float b = dot(ray_dir, light_to_cam);
	float c = dot(light_to_cam, light_to_cam) - (radius * radius);

	float d = sqrt((b*b) - c);
	float start = -b - d;
	float end = -b + d;

    float t = dot(light_to_cam, ray_dir); // point on the ray that is the closest to the sphere center

    vec3 pt = ray_orig + t * ray_dir;

    // find distance from t1 to t
    // x^2 + y^2 = R^2  <=> x = +/- sqrt(R^2 - y^2)
    float y = length(center - pt);

    vec3 acc = vec3(0.0);

    float linear_depth = linearize_depth(depth, 0.5, 50);
    float projected_depth = linear_depth / dot(cam_forward, ray_dir);
    // end = min(end, projected_depth);

    // if(y < radius)
    {
        float x = sqrt(radius*radius - y*y);

        float t1 = t - x;
        float t2 = t + x;

        vec3 pt1 = ray_orig + start * ray_dir;
        vec3 pt2 = ray_orig + end * ray_dir;

        // raymarch inside sphere
        vec3 ray_start = pt1;
        vec3 ray_end   = pt2;
        float ray_length = length(pt2 - pt1);

        const int num_steps = 25;
        float step_size = ray_length / float(num_steps);

        for(int i = 0; i < num_steps; i++)
        {
            vec3 light_dir = sphere_center - ray_start;
            float atten = atten_sphere_volume(length(light_dir), sphere_radius);
            light_dir = normalize(light_dir);
            acc += mie_scattering(dot(ray_dir, -light_to_cam), 0.9) * light_color * atten;
            ray_start += step_size * ray_dir;
        }

        return acc / num_steps;
    }

    return acc;
}

#endif // VOLUMETRIC_FOG_GLSL