
/*  
*   Microfacet model : specular and diffuse term
*   Specular term : Cook-Torrance model
*   Diffuse term : Lambertian model 
*/

const float PI = 3.1415926538;
const float INV_PI = 1.0 / PI;

/*
    Normal distribution function. Specular distribution term D.
    @param NoH
    @param perceptual roughness
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
vec3 Fd_Lambert(vec3 diffuse_reflectance)
{
    return diffuse_reflectance * INV_PI;
}

/* Unreal Shading */
float SpecularD(float NoH, float a)
{
    float a_sq = a * a;
    float d = (NoH * NoH) * (a_sq - 1.f) + 1.f;
    return a_sq / (PI * d*d);
}

float SpecularG(float NoV, float NoL, float r)
{
	float k = ((r + 1.f) * (r + 1.f)) / 8.f;
	float G_v = NoV / (NoV * (1.f - k) + k);
	float G_l = NoL / (NoL * (1.f - k) + k);
	return G_v * G_l;
}

vec3 SpecularF(vec3 F0, float HoV)
{	
	float e = (-5.55473 * HoV - 6.98316) * HoV;
	return F0 + (1.f-F0) * pow(2, e);
}

vec3 GetSpecularReflectance(vec3 base_color, float metalness)
{
    vec3 f0_dielectric = vec3(0.04);
    return mix(f0_dielectric, base_color, metalness);
}

struct BRDFData
{
    vec3 diffuse_reflectance;
    vec3 specular_reflectance;  // Specular F0 at normal incidence
};

vec3 BRDF(vec3 n, vec3 v, vec3 l, vec3 h, vec3 light_color, vec3 irradiance, vec3 base_color, float metalness, float perceptual_roughness)
{
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);
    float VoH = clamp(dot(v, h), 0.0, 1.0);

    vec3 final_color = vec3(0);

    /* Diffuse reflection */
    vec3 diffuse_reflectance = base_color * (1.0f - metalness);
    vec3 diffuse = Fd_Lambert(diffuse_reflectance);

    /* Specular reflection */
    // https://google.github.io/filament/Filament.md.html#table_commonmatreflectance
    vec3 specular_reflectance = GetSpecularReflectance(base_color, metalness);
    
    float D = SpecularD(NoH, perceptual_roughness);
    vec3  F = SpecularF(specular_reflectance, VoH);
    float G = SpecularG(NoV, NoL, perceptual_roughness);
    vec3 specular = (D * G * F) / (4 * NoL * NoV);

    final_color = irradiance * light_color * NoL * (diffuse + specular);

    return final_color;
}