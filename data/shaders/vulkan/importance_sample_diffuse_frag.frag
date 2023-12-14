#version 460

#include "headers/ibl_utils.glsl"

layout(set = 0, binding = 0) uniform sampler2D spherical_env_map;
layout(set = 0, binding = 1) uniform ParametersBlock
{
    uint k_env_map_width;               // Width of source environment map
    uint k_env_map_height;              // Height of source environment map
    uint num_samples_diffuse;           // Number of num_samples_diffuse for importance sampling. Default : 256
    uint base_mip_diffuse;              // Mipmap level of the environment map to sample from. Default: 0 
    uint mode;                          // Whether to prefilter diffuse or specular.
} params;

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 out_color;

layout(push_constant) uniform PSBlock
{
    uint current_mip_level;
} push_constants;


vec3 prefilter_env_map_diffuse(in sampler2D env_map)
{
    vec2 pixel_coord = uv_coord_to_pixel_coord(uv, uvec2(params.k_env_map_width, params.k_env_map_height));

    /* Compute normal vector corresponding to the env map texel at UV */
    vec3 normal = spherical_env_map_to_direction(uv);
    mat3 normal_space_to_world_space = get_normal_frame(normal);

    /* Sampling */
    uint N = params.num_samples_diffuse; 
    vec3 result = vec3(0.0);
    for(uint n = 0; n < N; n++)
    {
        /* Generate a random position with a uniform distribution in [0; 1] */
        // vec3 random_pixel_pos = pcg3d(uvec3(pixel_coord, n));
        vec2 random_pixel_pos = hammersley(n, N);

        /* Importance sampling */
        /* Convert to directions on the unit hemisphere */ 
        float phi = 2.0 * PI * random_pixel_pos.x;
        float theta = asin(sqrt(random_pixel_pos.y));

        /* Sample position in the local coordinate system of the normal */ 
        vec3 pos_local = spherical_coord_to_cartesian_coord(theta, phi);
        vec3 pos_world = normal_space_to_world_space * pos_local;

        /* Retrieve in the environment map the texel position corresponding the the world position of the sample */  
        vec2 uv_sampled_pos = SampleSphericalMap_YXZ(pos_world);

        /* Get radiance value at sampled pos */
        vec3 radiance = textureLod(env_map, uv_sampled_pos, params.base_mip_diffuse).rgb;

        result += radiance;
    }
    result = result / float(N);
    return result;
}


const uint  specular_filtering_per_mip_samples[6]   = { 512, 4096, 4096, 4096, 8096, 8096 };
const float specular_filtering_per_mip_roughness[6] = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0 };

vec3 prefilter_env_map_specular(in sampler2D env_map, float roughness)
{
    vec2 pixel_coord = uv_coord_to_pixel_coord(uv, uvec2(params.k_env_map_width, params.k_env_map_height));

    /* Compute normal vector corresponding to the env map texel at UV */
    vec3 normal = spherical_env_map_to_direction(uv);

    mat3 normal_space_to_world_space = get_normal_frame(normal);

    /* Assume normal vector and view vector have the same direction */
    vec3 view_dir = normal;
    /* Sampling */
    uint N = specular_filtering_per_mip_samples[push_constants.current_mip_level]; 
    vec3 result = vec3(0.0);
    float total_weight = 0.0;
    for(uint n = 0; n < N; n++)
    {
        /* Generate a random position with a uniform distribution in [0; 1] */
        // vec3 random_pixel_pos = pcg3d(uvec3(pixel_coord, n));
        vec2 random_pixel_pos = hammersley(n, N);

        /* Importance sampling */
        /* Convert to directions on the unit hemisphere */ 
        float phi = 2.0 * PI * random_pixel_pos.x;
        float u = random_pixel_pos.y;
        float alpha = roughness * roughness;

        /* Sample a random halfway vector */
        float theta = acos(sqrt((1.0 - u) / (1.0+(alpha*alpha - 1.0) * u)));   

        /* Halfway vector in the local coordinate system of the normal */ 
        vec3 halfvec_local = spherical_coord_to_cartesian_coord(theta, phi);
        vec3 halfvec_world = normal_space_to_world_space * halfvec_local;

        /* Find light direction from halfway vector */
        vec3 lightdir_world = 2.0 * dot(view_dir, halfvec_world) * halfvec_world - view_dir; 

        float NoL = dot(normal, lightdir_world);

        /* Only consider light directions above surface */
        if(NoL > 0.0)
        {
            /* Retrieve in the environment map the texel position corresponding the the world position of the sample */  
            vec2 uv_sampled_pos = SampleSphericalMap_YXZ(lightdir_world);

            /* Get radiance value at sampled pos */
            vec3 radiance = textureLod(env_map, uv_sampled_pos, push_constants.current_mip_level).rgb;
            result += radiance * NoL;
            total_weight += NoL;
        }
    }

    result = result / total_weight;
    return result;
}

void main()
{
    if(params.mode == 0)
    {
        out_color.rgb = prefilter_env_map_diffuse(spherical_env_map);
    }
    else if(params.mode == 1)
    {
        out_color.rgb = prefilter_env_map_specular(spherical_env_map, specular_filtering_per_mip_roughness[push_constants.current_mip_level]);
    }
}

