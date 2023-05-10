
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
    float NoV = clamp(abs(dot(n, v)), 0.00390625, 1.0);
    float NoL = clamp(dot(n, l), 0.00390625, 1.0);
    float NoH = clamp(dot(n, h), 0.00390625, 1.0);
    float LoH = clamp(dot(l, h), 0.00390625, 1.0);
    float HoV = clamp(dot(h, v), 0.00390625, 1.0);
    vec3 F0 = vec3(0.04);

    float roughness = perceptual_roughness * perceptual_roughness;
    vec3 specular_color = mix(F0, albedo, metallic);
    float reflectance = maxVec3(specular_color);
    vec3 reflectance90 = vec3(1.0,1.0,1.0) * clamp(reflectance * 25.0, 0.0, 1.0);

    vec3 diffuse_color  = albedo * (vec3(1.0) - F0);
    diffuse_color *= 1.0 - metallic;

    float D = D_GGX(NoH, roughness);
    vec3  F = F_Schlick(LoH, F0);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

    vec3 Fd = (1.0 - F) * (diffuse_color * Fd_Lambert());
    vec3 Fr = (D * V) * F;

    vec3 color = NoL * light_color * (Fd + Fr);

    return color;
}

/////////////////////////////////////////////////////////

float DistributionGGX (vec3 N, vec3 H, float roughness){
    float a2    = roughness * roughness * roughness * roughness;
    float NdotH = max (dot (N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometrySchlickGGX (float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith (vec3 N, vec3 V, vec3 L, float roughness){
    return geometrySchlickGGX (max (dot (N, L), 0.0), roughness) * 
           geometrySchlickGGX (max (dot (N, V), 0.0), roughness);
}

vec3 fresnelSchlick (float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow (1.0 - cosTheta, 5.0);
}

vec3 BRDF_OGL(vec3 N, vec3 V, vec3 L, vec3 H, vec3 light_color, vec3 albedo, float metallic, float perceptual_roughness)
{
    vec3 radiance     = light_color;        
    float roughness = perceptual_roughness * perceptual_roughness;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    vec3 Lo =  (kD * albedo / PI + specular) * radiance * NdotL; 

    return Lo;
}