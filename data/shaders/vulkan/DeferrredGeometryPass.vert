#version 460
#include "Headers/LightDefinitions.glsl"
#include "Headers/VertexDefinitions.glsl"

layout(location = 0) out vec4 uv;
layout(location = 1) out vec4 normalWS;
layout(location = 2) out vec4 positionCS;
layout(location = 3) out vec4 depthCS;
layout(location = 4) out vec4 positionWS;
layout(location = 5) out vec3 vertexToEye;
layout(location = 6) out mat3 TBN;

layout(set = 0, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
layout(set = 0, binding = 1) readonly buffer IBO { uint data[]; } ibo;

layout(set = 1, binding = 0) uniform ObjectData 
{ 
    mat4 mvp;
    mat4 model;
    vec4 bbox_min_WS;
    vec4 bbox_max_WS;
} object_data;

layout(set = 3, binding = 0) uniform FrameData 
{ 
    mat4 view;
    mat4 proj;
    mat4 inv_view_proj;
    vec4 view_pos;
} frame_data;

layout(push_constant) uniform PushConstant
{
    mat4 model;
} pushConstant;

void main()
{
    uint index = ibo.data[gl_VertexIndex];
    Vertex v = vbo.data[index];
    vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);
    vec4 normalOS   = vec4(v.nx, v.ny, v.nz, 0.0);

    vec3 tangentWS = normalize(vec3(pushConstant.model * vec4(v.tx, v.ty, v.tz, 0)));
    normalWS       = normalize(pushConstant.model * normalOS);
    vec3 bitangentWS = cross(vec3(normalWS), tangentWS);

    TBN = mat3(tangentWS, bitangentWS, normalWS);

    uv.xy = vec2(v.u, v.v);
    positionCS =  frame_data.proj * frame_data.view * pushConstant.model * positionOS;
    depthCS.xy = positionCS.zw;
    positionWS = pushConstant.model * positionOS;

    vertexToEye = normalize(frame_data.view_pos - positionWS).xyz;

    gl_Position = positionCS;
}