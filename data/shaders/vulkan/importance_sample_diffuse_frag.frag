#version 460

#include "headers/ibl_utils.glsl"

layout(set = 0, binding = 0) uniform sampler2D spherical_env_map;
layout(set = 0, binding = 1) uniform ParametersBlock
{
    uint k_env_map_width;
    uint k_env_map_height;
    uint samples;           // Number of samples for importance sampling. Default : 256
    uint mipmap_level;      // Mipmap level of the environment map to sample from. Default: 0 
} params;

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 out_color;

vec3 prefilter_env_map_diffuse(in sampler2D env_map)
{
    vec2 pixel_coord = uv_coord_to_pixel_coord(uv, uvec2(params.k_env_map_width, params.k_env_map_height));

    /* Compute normal vector corresponding to the env map texel at UV */
    vec3 normal = spherical_env_map_to_direction(uv);
    mat3 normal_space_to_world_space = get_normal_frame(normal);

    /* Sampling */
    uint N = params.samples; 
    vec3 result = vec3(0.0);
    for(uint n = 0; n < N; n++)
    {
        /* Generate a random position with a uniform distribution in [0; 1] */
        vec3 random_pixel_pos = pcg3d(uvec3(pixel_coord, n));

        /* Importance sampling */
        /* Convert to directions on the unit hemisphere */ 
        float phi = 2.0 * PI * random_pixel_pos.x;
        float theta = asin(sqrt(random_pixel_pos.y));

        /* Sample position in the local coordinate system of the normal */ 
        vec3 pos_local = spherical_coord_to_cartesian_coord(theta, phi);
        vec3 pos_world = normal_space_to_world_space * pos_local;

        /* Retrieve in the environment map the texel position corresponding the the world position of the sample */  
        vec2 uv_sampled_pos = direction_to_spherical_env_map(pos_world);

        /* Get radiance value at sampled pos */
        vec3 radiance = textureLod(env_map, uv_sampled_pos, params.mipmap_level).rgb;

        result += radiance;
    }
    result = result / float(N);
    return result;
}

void main()
{
    vec3 color = prefilter_env_map_diffuse(spherical_env_map);
    vec3 gamma = pow(color, vec3(2.2)); 

    out_color = vec4(gamma, 1.0);
}