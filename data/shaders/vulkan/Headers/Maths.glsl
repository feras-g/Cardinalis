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

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv ) 
{ 
  // get edge vectors of the pixel triangle 
  vec3 dp1 = dFdx( p ); vec3 dp2 = dFdy( p ); 
  vec2 duv1 = dFdx( uv ); vec2 duv2 = dFdy( uv );   

  // solve the linear system 
  vec3 dp2perp = cross(N, dp2 ); 
  vec3 dp1perp = cross(dp1, N); 
  vec3 T = dp2perp * duv1.x + dp1perp * duv2.x; 
  vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

  // construct a scale-invariant frame 
  float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) ); 
  return mat3( T * invmax, B * invmax, N ); 
}

vec3 perturb_normal( vec3 N, vec3 N_map, vec3 V, vec2 texcoord )
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
    N_map.y = -N_map.y; // gltf normal maps use right-handed coordinates (OpenGL convention)
    mat3 TBN = cotangent_frame( N, -V, texcoord );
    return normalize( TBN * N_map );
}



