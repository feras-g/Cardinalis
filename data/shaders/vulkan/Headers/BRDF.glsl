
/*  
*   Microfacet model : diffuse + specular term
*   Specular term : Cook-Torrance model
*   Diffuse term : Lambertian model 
*/

const float PI = 3.1415926538;
const float INV_PI = 1.0 / PI;

/* 
* https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf  
* https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf
*/

/* Diffuse term */

float brdf_lambert()
{
    return INV_PI;
}

/* Specular term */

/* 
*   Microfacet distribution function and is responsible for the shape of the specular peak
*   GGX/Trowbridge-Reitz NDF 
*/
float specular_D(float NoH, float roughness)
{
    float alpha = roughness * roughness; /* Disney reparametrization */
    float alpha_sq = alpha * alpha;

    float d = (NoH * NoH) * (alpha_sq - 1.f) + 1.f;
    return alpha_sq / (PI * d*d);
}

/* 
*   Fresnel reflection coefficient
*/
vec3 specular_F(vec3 F0, float VoH)
{	
	float e = (-5.55473 * VoH - 6.98316) * VoH;
	return F0 + (1 - F0) * pow(2, e);
}

/* 
*   Geometric attenuation/Shadowing factor
*/
float specular_G(float NoV, float NoL, float roughness)
{
	float k = ((roughness + 1.f) * (roughness + 1.f)) / 8.f;
	float G_v = NoV / (NoV * (1.f - k) + k);
	float G_l = NoL / (NoL * (1.f - k) + k);
	return G_v * G_l;
}

/*
*   Diffuse reflectance represents light that is refracted into the surface, scattered, partially absorbed, and re-emitted.
*
*/
vec3 get_diffuse_reflectance(vec3 base_color, float metalness)
{
    return base_color * (1.0f - metalness);
}

vec3 get_specular_reflectance(vec3 base_color, float metalness)
{
    vec3 f0_dielectric = vec3(0.04);
    return mix(f0_dielectric, base_color, metalness);
}

struct BRDFData
{
    vec3 diffuse_reflectance;
    vec3 specular_reflectance;  // Specular F0 at normal incidence
};

vec3 brdf_cook_torrance(vec3 n, vec3 v, vec3 l, vec3 h, vec3 light_color, vec3 irradiance, vec3 base_color, float metalness, float roughness)
{
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 1e-5, 1.0);
    float NoH = clamp(dot(n, h), 1e-5, 1.0);
    float LoH = clamp(dot(l, h), 1e-5, 1.0);
    float VoH = clamp(dot(v, h), 1e-5, 1.0);

    /* Diffuse reflection */
    vec3 diffuse_reflectance = get_diffuse_reflectance(base_color, metalness);
    vec3 diffuse = diffuse_reflectance * brdf_lambert();

    /* Specular reflection */
    // https://google.github.io/filament/Filament.md.html#table_commonmatreflectance
    vec3 specular_reflectance = get_specular_reflectance(base_color, metalness);
    
    float D = specular_D(NoH, roughness);
    vec3  F = specular_F(specular_reflectance, VoH);
    float G = specular_G(NoV, NoL, roughness);
    vec3 specular = (D * G * F) / (4 * NoL * NoV);

    return (diffuse + specular) * light_color * NoL;
}

/* To multiply with specular color */
vec3 brdf_blinn_phong(vec3 diffuse_reflectance, vec3 specular_reflectance, vec3 n, vec3 l, vec3 h, float shininess)
{
    float NoL = clamp(dot(n, l), 1e-5, 1.0);
    float NoH = clamp(dot(n, h), 1e-5, 1.0);

    float blinn = pow(max(dot(n, h), 0), shininess);

    vec3 specular = blinn * specular_reflectance;
    return (diffuse_reflectance * NoL);
}