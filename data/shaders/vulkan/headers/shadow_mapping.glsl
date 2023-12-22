const uint max_cascades = 4;

struct CascadesData
{
    mat4 dir_light_view_proj[max_cascades];
    float distances[max_cascades];
    uint num_cascades;
	bool show_debug_view;
};

const mat4 bias_matrix = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);


// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
float sample_shadow_map_pcf(vec3 light_space_pos, uint cascade_index, sampler2DArray shadow_maps_array)
{
	float acc = 0.0f;
	float bias = 0.005;
	for(float i = -1.5; i <= 1.5; i+=1.0)
	{
		for(float j = -1.5; j <= 1.5; j+=1.0)
		{
			float shadow_map_depth = texture(shadow_maps_array, vec3(light_space_pos.st + (vec2(i, 1-j) * vec2(1.0/2048.0)), cascade_index)).r;
			if (shadow_map_depth + bias < light_space_pos.z) 
			{
				acc += shadow_map_depth;
			}
		}
	}

	return  pow(1-acc / 16.0, 2);
}



float sample_shadow_map(vec3 light_space_pos, uint cascade_index, sampler2DArray shadow_maps_array)
{
	float shadow = 1.0;
	float bias = 0.005;

	float shadow_map_depth = texture(shadow_maps_array, vec3(light_space_pos.st, cascade_index)).r;
	if (shadow_map_depth + bias < light_space_pos.z) 
	{
		shadow = 0.27;
	}
	return shadow;
}

vec4 GetShadowFactor(vec3 position_ws, float depth_vs, CascadesData data, sampler2DArray shadow_maps_array)
{

	int cascade_index = 0;
	for(int i = 0; i < data.num_cascades - 1; i++)
	{
		if(depth_vs < data.distances[i]) 
		{	
			cascade_index = i + 1;
		}
	}

	
	vec4 out_val = vec4(0);

	{
		switch(cascade_index) 
		{
			case 0 : 
				out_val.rgb = vec3(1.0f, 0.25f, 0.25f);
				break;
			case 1 : 
				out_val.rgb = vec3(0.25f, 1.0f, 0.25f);
				break;
			case 2 : 
				out_val.rgb = vec3(0.25f, 0.25f, 1.0f);
				break;
			case 3 : 
				out_val.rgb = vec3(1.0f, 1.0f, 0.25f);
				break;
		}
	}

    vec4 light_space_ndc = data.dir_light_view_proj[cascade_index] * vec4(position_ws, 1.0);

	// do perspective division and scale from NDC XY [-1, 1] to UV [0, 1]
	light_space_ndc /= light_space_ndc.w;
	light_space_ndc.xy = light_space_ndc.xy * 0.5 + 0.5;
	
	vec3 shadow_coord = vec3(light_space_ndc.xy, light_space_ndc.z);
	shadow_coord.y = 1 - shadow_coord.y;
    //out_val.a = sample_shadow_map(shadow_coord, cascade_index, shadow_maps_array);
	out_val.a = sample_shadow_map_pcf(shadow_coord, cascade_index, shadow_maps_array);
	return out_val;
}


