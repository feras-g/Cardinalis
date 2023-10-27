// Normal Mapping Without Precomputed Tangents 
// http://www.thetenthplanet.de/archives/1180

mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv) 
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

vec3 perturb_normal(vec3 normal, vec3 normal_map, vec3 vertex_to_eye, vec2 texcoord)
{
    // assume N (normal), the interpolated vertex normal and 
    // V (vertex_to_eye), the view vector (vertex to eye)
    normal_map.y = -normal_map.y; // gltf normal maps use right-handed coordinates (OpenGL convention)
    mat3 TBN = cotangent_frame( normal, -vertex_to_eye, texcoord );
    return normalize( TBN * normal_map );
}