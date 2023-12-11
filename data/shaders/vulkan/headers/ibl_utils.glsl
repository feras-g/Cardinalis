/* 
    Convert texture coordinates to pixel coordinates.

    @params uv The uv coordinate.
    @params image_size The image size in pixels.    
*/

#include "math_constants.glsl"

vec2 uv_coord_to_pixel_coord(in vec2 uv, in uvec2 image_size)
{
    return (vec2(uv.x, uv.y) * image_size) - vec2(0.5, 0.5); // shift by 0.5 to be at pixel center
}

/* 
    Hash Functions for GPU Rendering
    https://jcgt.org/published/0009/03/02/paper.pdf 

    @params v The seed for the random number generator.

    Returns 3 random numbers having a uniform distribution from [0, 1]
*/
vec3 pcg3d(in uvec3 v) 
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    v ^= v >> 16u;
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    return vec3(v) * (1.0/float(0xffffffffu));
}

/* 
    Convert texture coordinates to spherical coordinates. 

    @params uv The uv coordinate.
*/
vec2 uv_to_spherical(in vec2 uv)
{
    // s is mapped from [0.0; 1.0] to [+PI; -PI] to get azimuth angle phi
    // t is mapped from [0.0; 1.0] to [+PI; 0] to get polar angle theta
    float phi = 2.0 * PI * (0.5 - uv.s);
    float theta = PI * (1.0 - uv.t);

    return vec2(theta, phi);
}

vec3 spherical_coord_to_cartesian_coord(in float theta, in float phi)
{
    vec3 cartesian;
    cartesian.x = sin(theta) * cos(phi);
    cartesian.y = sin(theta) * sin(phi);
    cartesian.z = cos(theta);
    return cartesian;
}

/* 
    From a texture coordinate, retrieve the direction
    of the normal to a point on the spherical env map.

    @params uv The texture coordinate of a point on the spherical env map.
*/
vec3 spherical_env_map_to_direction(vec2 uv)
{
    /* Convert texture coordinate to spherical */
    vec2 spherical = uv_to_spherical(uv);

    /* 
        Convert spherical coordinate to cartesian.
        This gives us a point on the unit sphere. 
        The vector from the sphere's center (0,0,0) to this point is the normal vector. 
    */
    vec3 cartesian = spherical_coord_to_cartesian_coord(spherical.x, spherical.y);

    return cartesian;
}

vec2 SampleSphericalMap_ZXY(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.x, v.z), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return vec2(uv.x, uv.y);
}

vec2 SampleSphericalMap_YXZ(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.y, v.x), asin(v.z));
    uv *= invAtan;
    uv += 0.5;
    return vec2(uv.x, uv.y);
}

/* 
    From a direction on the unit sphere, retrieve
    the corresponding position on the env map

    @params d The direction on the unit sphere.
*/
vec2 direction_to_spherical_env_map(vec3 dir)
{
    /* Cartesian to spherical */
    // float phi = atan(dir.z, dir.x);
    // float theta = acos(dir.y);

    // /* Spherical to texture coordinate */
    // float s = 0.5 - phi / (2.0 * PI);
    // float t = 1.0 - theta / PI;
    // return vec2(s, t);

    // const vec2 invAtan = vec2(0.1591, 0.3183);
    // vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    // uv *= invAtan;
    // uv += 0.5;
    // return uv;

	return vec2(0.5f + 0.5f * atan(dir.z, dir.x) / PI,1.f - acos(dir.y) / PI);
}

mat3 get_normal_frame(in vec3 normal)
{
    vec3 arbitrary_vec = vec3(1.0, 0.0, 0.0);
    vec3 tangent;
    if( 1.0 - abs(dot(arbitrary_vec, normal)) <= 1e-6)
    {
        /* Problem: arbitrary_vec and normal are parallel, cannot do cross product. */
        arbitrary_vec = vec3(0.0, 1.0, 0.0);
    }
    tangent = normalize(cross(arbitrary_vec, normal));
    vec3 bitangent = cross(normal, tangent);
    return mat3(tangent, bitangent, normal);
}

float GGX_Schlick(float NoV, float roughness)
{
    float alpha = roughness * roughness;
    float k = alpha * 0.5;
    return max(NoV, 0.001) / (NoV * (1.0 - k) + k);
}

float G_Smith(float NoV, float NoL, float roughness)
{
    return GGX_Schlick(NoL, roughness) * GGX_Schlick(NoV, roughness);
}