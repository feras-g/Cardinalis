#ifndef LIGHTS_GLSL // LIGHTS_GLSL
#define LIGHTS_GLSL

struct PointLight
{
	vec3 position;
	float radius;
	vec3 color;
	float pad1;
};

struct DirectionalLight
{
    vec4 dir;
    vec4 color;
};

struct Spotlight
{
    vec3 pos;
    vec3 dir;
    vec3 color;

    float angle_umbra;
    float angle_penumbra;
    float falloff;
    float cos_cutoff_angle; // cosine of angle between spotlight direction and spotlight cone "edge"
};

/* 
    Returns the light attenuation for a point light. 
    @param d: The distance between the point light and the shaded point.

    Note: Singularity at d=0

*/
float attenuation(float d)
{
    return 1.0f / (d*d);
}

/* 
    Returns the light attenuation for a point light of radius r. 
    @param d: The distance between the point light and the shaded point.
    @param r: The point light radius.
*/
float attenuation(float d, float r)
{
    float d_r = (d / r);
    return 1.0f / (1 + (d_r * d_r));
}

float attenuation_unity(float d, float r)
{
    float d_r = (d / r) * 5;
    return 1.0f / (1 + (d_r * d_r));
}

float attenuation_frostbite(float d, float r)
{
    float m = max(d, r);
    return 1.0 / (m*m);
}

float attenuation_gltf(float d, float r)
{
    /* https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property */
    return max( 0, min( 1.0 - pow( d / r, 4), 1 ) ) / (d*d);
}

float atten_sphere_volume(float dist, float radius)
{
    float att = clamp(1.0 - dist/radius, 0.0, 1.0);
    att *= att;
	return att;
}

#endif // LIGHTS_GLSL