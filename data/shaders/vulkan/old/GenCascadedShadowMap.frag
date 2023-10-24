#version 460
#include "Headers/LightDefinitions.glsl"
#include "Headers/Maths.glsl"

layout(location=0) out float out_depth;
layout(location=1) in vec4 pos_LS;
void main()
{
    out_depth = gl_FragCoord.z;
}