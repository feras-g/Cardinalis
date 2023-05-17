#version 460

layout(set = 2, binding = 0) uniform samplerCube cubemap;

layout(location = 0) in vec4 positionOS;
layout(location = 0) out vec4 out_color;

const float PI = 3.14159265359;

void main()
{
    vec3 normal = normalize(positionOS.xyz);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

	vec3 irradiance = vec3(0.0);
	float sampleDelta = 0.025;
	float nrSamples = 0.0; 

	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
		{
			vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));	
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;		

			irradiance += texture(cubemap, sampleVec).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	
	irradiance = PI * irradiance * (1.0 / float(nrSamples));
	out_color = vec4(irradiance, 1);
}

