vec3 fog(vec3 rgb, float distance, float b)
{
    float fogAmount = 1.0 - exp( -distance * b );
    vec3  fogColor  = vec3(0.5,0.6,0.7);
    return mix( rgb, fogColor, fogAmount );
}