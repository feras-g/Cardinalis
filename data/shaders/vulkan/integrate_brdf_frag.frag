#version 460

#include "headers/ibl_utils.glsl"

layout (location = 0) in vec2 uv;
layout (location = 0) out vec2 out_color;

// Real Shading in Unreal Engine 4, page 7, https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
vec2 integrate_brdf(float NoV, float roughness)
{
    vec2 pixel_coord = uv_coord_to_pixel_coord(uv, uvec2(512, 512));

    /* View direction in spherical coordinates */
    float view_theta = acos(NoV);
    // Phi can be chosen arbitrarly. We choose Phi = 0, so that cos(0) = 1, sin(0) = 0

    /* Normal space view direction in cartesian coordinates, from the view in spherical coordinates */
    // Choosing Phi=0 simplifies the conversion to cartesian coordinates. 
    vec3 viewdir_local = vec3(sin(view_theta), 0.0, cos(view_theta));

    /* Sampling */
    uint N = 1024; 
    vec2 result = vec2(0.0);
    for(uint n = 0; n < N; n++)
    {
        /* Generate a random position with a uniform distribution in [0; 1] */
        vec3 random_pixel_pos = pcg3d(uvec3(pixel_coord, n));

        /* Importance sampling */
        /* Convert to directions on the unit hemisphere */ 
        float phi = 2.0 * PI * random_pixel_pos.x;
        float u = random_pixel_pos.y;
        float alpha = roughness * roughness;

        /* Sample a random halfway vector */
        float theta = acos(sqrt((1.0 - u) / (1.0+(alpha*alpha - 1.0) * u)));   

        /* Halfway vector in the local coordinate space of the normal */ 
        vec3 halfvec_local = spherical_coord_to_cartesian_coord(theta, phi);

        /* Find light direction in coordinate space of normal from halfway vector */
        vec3 lightdir_local = 2.0 * dot(viewdir_local, halfvec_local) * halfvec_local - viewdir_local;

        /* Normal points in Z direction in normal space (0,0,1), so NoX = X.z where X is any vec3*/
        float NoL = clamp(lightdir_local.z, 0.0, 1.0);
        float NoH = clamp(halfvec_local.z, 0.0, 1.0);
        float VoH = clamp(dot(viewdir_local, halfvec_local), 0.0, 1.0);

        /* BRDF */
        if(NoL > 0.0)
        {
            float G = G_Smith(NoV, NoL, roughness);
            float G_vis = G * VoH / (NoH * NoV);
            float Fc = pow(1.0 - VoH, 5.0);
            result.x += (1.0 - Fc) * G_vis;
            result.y += Fc * G_vis;
        }
    }

    result = result / N;
    return result;
}

void main()
{
    out_color = integrate_brdf(uv.x, 1-uv.y);
}