#ifndef SHADOW_MAPPING_GLSL
#define SHADOW_MAPPING_GLSL

const uint max_cascades = 4;

struct CascadesData
{
    mat4 dir_light_view_proj[max_cascades];
    float distances[max_cascades];
    uint num_cascades;
	bool show_debug_view;
};

float lookup_shadow(sampler2DArray tex_shadow, vec4 position_light_space, vec2 offset, uint layer)
{
	// Perspective divison + scale bias
	vec4 shadow_coord = position_light_space / position_light_space.w;
	vec2 uv = shadow_coord.xy * 0.5 + 0.5;
	uv.y = 1 - uv.y;

	const float bias = 0.005;
	float shadow_map_depth = texture(tex_shadow, vec3(uv + offset, layer)).r;

	if (shadow_map_depth + bias < position_light_space.z) 
	{
		return 0.0;
	}
	return 1.0f;
}

// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
float filter_shadow_pcf(sampler2DArray tex_shadow, vec4 position_light_space, uint layer)
{
	float acc = 0.0f;
	float bias = 0.005;
	const vec2 scale = vec2(1.0 / 2048.0);
	for (float y = -1.5; y <= 1.5; y += 1.0)
	{
		for (float x = -1.5; x <= 1.5; x += 1.0)
		{
			acc += lookup_shadow(tex_shadow, position_light_space, vec2(x,y) * scale, layer);
		}
	}
	return acc / 16.0;
}

int interval_based_selection(in CascadesData data, in float depth_vs)
{
	int cascade_index = 0;
	for(int i = 0; i < data.num_cascades - 1; i++)
	{
		if(depth_vs < data.distances[i]) 
		{	
			cascade_index = i + 1;
		}
	}
	return cascade_index;
}

/* 
	Returns the view-projection matrix correponding to the current cascade 
*/
mat4 get_cascade_view_proj(in float depth_vs, in CascadesData data, inout int cascade_index)
{
	cascade_index = interval_based_selection(data, depth_vs);
	return data.dir_light_view_proj[cascade_index];
}

float get_shadow_factor(sampler2DArray tex_shadow, vec4 position_light_space, int cascade_index)
{
	//return lookup_shadow(tex_shadow, position_light_space, vec2(0), cascade_index);
	return filter_shadow_pcf(tex_shadow, position_light_space, cascade_index);
}
#endif // SHADOW_MAPPING_GLSL


