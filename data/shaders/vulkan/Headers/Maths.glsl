/* Returns the component-wise maximum */
float maxVec3 (vec3 v) 
{
  return max (max (v.x, v.y), v.z);
}

/* Returns world space position from depth */
vec3 ws_pos_from_depth(vec2 uv, float z, mat4 inv_view_proj)
{
    // z is NDC depth

    // Compute NDC position
    float x = uv.x * 2 - 1;
	  float y = (1-uv.y) * 2 - 1;
    vec4 ndc_pos = vec4(x,y,z, 1.0);

    // NDC -> View-Space
    // After applying inverse view proj, vector is still in projective space,
    // this is why we divide by W
    vec4 ws_pos = inv_view_proj * ndc_pos; 
    ws_pos /= ws_pos.w;

    return ws_pos.xyz;
}


// Source: https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli
vec3 ApplySRGBCurve( vec3 x )
{
  // Approximately pow(x, 1.0 / 2.2)
  return all(lessThan(x, vec3(0.0031308))) ? 12.92 * x : 1.055 * pow(x, vec3(1.0 / 2.4)) - 0.055;
}


