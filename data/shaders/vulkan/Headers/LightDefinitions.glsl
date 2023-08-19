struct PointLight
{
    vec4 position;
    vec4 color;
    float radius;
};

struct DirectionalLight
{
    vec4 direction;
    vec4 color;
};

/* 
    Returns the light attenuation for a point light. 
    @param d: The distance between the point light and the shaded point.

    Note: Singularity at d=0

*/
float attenuation(float d)
{
    return 1.0f / (d*d);
}

/* 
    Returns the light attenuation for a point light of radius r. 
    @param d: The distance between the point light and the shaded point.
    @param r: The point light radius.
*/
float attenuation(float d, float r)
{
    float d_r = (d / r);
    return 1.0f / (1 + (d_r * d_r));
}