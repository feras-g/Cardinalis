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
