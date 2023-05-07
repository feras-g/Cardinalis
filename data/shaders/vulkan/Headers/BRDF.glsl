
// Microfacet model : Specular term and Diffuse term
// Specular term : Cook-Torrance model 
// Diffuse term : Lambertian model

#define PI 3.1415926538

/* 
    Normal distribution function. Specular distribution term D.
    @param NoH 
    @param roughness
*/
float D_GGX(float NoH, float roughness) 
{
    float a = NoH * roughness;
    float k = roughness / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}

/* 
    Geometric shadowing and masking. Specular visibility term V.
    @param NoV 
    @param NoL
    @param roughness
*/
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) 
{
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

/* 
    Fresnel term. Schlick approximation.
    @param u 
    @param f0 Reflectance at normal incidence. 
    
*/
vec3 F_Schlick(float u, vec3 f0) 
{
    /* Reflectance at grazing angle (f90) is 1 for dielectrics and conductors */ 
    float f = pow(1.0 - u, 5.0);
    return f + f0 * (1.0 - f);
}

/* Diffuse BRDF */
float Fd_Lambert() 
{
    return 1.0 / PI;
}

vec3 BRDF(vec3 n, vec3 v, vec3 l, vec3 h, vec3 light_color, vec3 albedo, float metallic, float perceptual_roughness)
{
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.01, 1.0);
    float NoH = clamp(dot(n, h), 0.00390625, 1.0);
    float LoH = clamp(dot(l, h), 0.00390625, 1.0);

    float roughness = perceptual_roughness * perceptual_roughness;

    vec3 F0 = vec3(0.04);
    vec3 specular_color = mix(F0, albedo, metallic);

    float D = D_GGX(NoH, roughness);
    vec3  F = F_Schlick(LoH, F0);
    float V = V_SmithGGXCorrelated(NoV, NoL, perceptual_roughness);

    vec3 diffuseColor = (1.0 - metallic) * albedo;

    vec3 Fr = specular_color * (D * V) * F;
    vec3 Fd = diffuseColor * Fd_Lambert();

    vec3 color = Fd + Fr * NoL;
    color *= light_color;

    return color;
}