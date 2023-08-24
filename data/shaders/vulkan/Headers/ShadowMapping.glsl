
const mat4 bias_matrix = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float sample_shadow_map(vec4 shadow_coord, vec2 offset, uint cascade_index, sampler2DArray shadow_maps_array)
{
	float shadow = 1.0;
	float bias = 0.001;
	
	if ( shadow_coord.z > -1.0 && shadow_coord.z < 1.0 ) {
		float dist = texture(shadow_maps_array, vec3(shadow_coord.st + offset, cascade_index)).r;
		if (shadow_coord.w > 0 && dist < shadow_coord.z - bias) {
			shadow = 0.3;
		}
	}
	return shadow;
}

float sample_shadow_map_pcf(vec4 shadow_coord, uint cascade_index, sampler2DArray shadow_maps_array)
{
	const float texel_size = 1.0f / 2048.0f;
	float acc = 0.0f;
	float bias = 0.005;
	for(float x=-1.5; x <=1.5; x+=1.0)
	{
		for(float y=-1.5; y <= 1.5; y+=1.0)
		{
			vec2 offset = vec2(x, y) * texel_size;
			float shadow_depth = texture(shadow_maps_array, vec3(shadow_coord.st + offset, cascade_index)).r;
			if (shadow_coord.w > 0 && shadow_depth + bias >= shadow_coord.z) 
			{
				acc += 1.0f;
			}
			
		}
	}

	return acc / 16.0f;
}

float GetShadowFactor(vec3 ws_position, float vs_depth, vec4 cascade_splits, mat4 cascade_projs[4], sampler2DArray shadow_maps_array)
{
	uint cascade_index = 0;
	for(uint i = 0; i < 4 - 1; ++i) 
    {
		if(vs_depth <= cascade_splits[i]) 
        {	
			cascade_index = i + 1;
		}
	}

    vec4 shadow_coord = (bias_matrix * cascade_projs[cascade_index]) * vec4(ws_position, 1.0);
	shadow_coord.y = 1-shadow_coord.y;
    //return sample_shadow_map(shadow_coord / shadow_coord.w, vec2(0, 0), cascade_index, shadow_maps_array);
    return sample_shadow_map_pcf(shadow_coord / shadow_coord.w, cascade_index, shadow_maps_array);
}

