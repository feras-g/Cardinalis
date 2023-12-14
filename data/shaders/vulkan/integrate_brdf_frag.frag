#version 460

#include "headers/ibl_utils.glsl"

layout (location = 0) in vec2 uv;
layout (location = 0) out vec2 out_color;

// Real Shading in Unreal Engine 4, page 7, https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
/*
    roughness : Surface roughness
    dir :       Random 2D vector
    N :         Surface normal
*/

vec3 importance_sample_ggx(vec2 dir, float roughness, vec3 N)
{
    float a = roughness * roughness;

    /* Spherical coordinate of the random direction in the unit hemisphere */
    float phi = 2 * PI * dir.x;
    float cos_theta = sqrt( (1 - dir.y) / (1+(a*a - 1) * dir.y) );
    float sin_theta = sqrt(1-cos_theta * cos_theta);

    /* Cartesian coordinates of halfway vector in tangent space */
    vec3 H;
    H.x = sin_theta * cos(phi);
    H.y = sin_theta * sin(phi);
    H.z = cos_theta;

    /* Change of basis matrix */
    vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1, 0, 0);
    vec3 tangent_x = normalize(cross(up, N));
    vec3 tangent_y = cross(N, tangent_x);

    /* Halfway vector in world space */
    return tangent_x * H.x + tangent_y * H.y + N * H.z;
}

vec2 integrate_brdf(float NoV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0f - NoV * NoV);
    V.y = 0.0f;
    V.z = NoV;

    const uint num_samples = 1024; 
    vec2 result = vec2(0.0);
    for(uint i = 0; i < num_samples; i++)
    {
        /* Generate a random position with a uniform distribution in [0; 1] */
        vec2 random_dir = hammersley(i, num_samples);
        vec3 N = vec3(0, 0, 1);                                       /* Normal points in Z direction in tangent/normal space */
        vec3 H = importance_sample_ggx(random_dir, roughness, N);   /* Halfway vector in tangent space */     
        vec3 L = 2 * dot(V, H) * H - V;                             /* Light vector in tangent space */
        
        /* Because normal is (0,0,1) in tangent space, we always have dot(N, X) = X.z, where X is any vec3 */
        float NoL = clamp(L.z, 0.0, 1.0);  
        float NoH = clamp(H.z, 0.0, 1.0);
        float VoH = clamp(dot(V, H), 0.0, 1.0);

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

    result = result / num_samples;
    return result;
}

vec2 integrate_brdf_orig(float NoV, float roughness)
{
    /* View direction in spherical coordinates */
    float view_theta = acos(NoV);
    // Phi can be chosen arbitrarly. We choose Phi = 0, so that cos(0) = 1, sin(0) = 0

    /* Normal space view direction in cartesian coordinates, from the view in spherical coordinates */
    // Choosing Phi=0 simplifies the conversion to cartesian coordinates. 
    vec3 viewdir_local = vec3(sin(view_theta), 0.0, cos(view_theta));

    float a = roughness * roughness;

    /* Sampling */
    uint N = 1024; 
    vec2 result = vec2(0.0);
    for(uint n = 0; n < N; n++)
    {
        /* Generate a random position with a uniform distribution in [0; 1] */
        vec2 random_dir = hammersley(n, N);

        /* Importance sampling */
        /* Convert to directions on the unit hemisphere */ 
        float phi = 2.0 * PI * random_dir.x;

        /* Sample a random halfway vector */
        float theta = acos(sqrt((1.0 - random_dir.y) / (1.0+(a*a - 1.0) * random_dir.y)));   

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
    out_color = integrate_brdf(uv.x, uv.y);
}


