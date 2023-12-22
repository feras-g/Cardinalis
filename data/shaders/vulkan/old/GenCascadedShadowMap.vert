// #version 460
// #extension GL_EXT_multiview : enable

// #include "Headers/VertexDefinitions.glsl"
// #include "Headers/LightDefinitions.glsl"
// #include "Headers/ShadowMapping.glsl"

// layout(set = 0, binding = 0) readonly buffer VBO { Vertex data[]; } vbo;
// layout(set = 0, binding = 1) readonly buffer IBO { uint data[]; } ibo;

// layout(set = 1, binding = 0) readonly buffer ShadowCascadesSSBO
// {
//     CascadeData data;
// } shadow_cascades;

// layout(push_constant) uniform PushConstants
// {
//     mat4 model;
// } mat;

// void main()
// {
//     uint index = ibo.data[gl_VertexIndex];
//     Vertex v = vbo.data[index];
//     vec4 positionOS = vec4(v.px, v.py, v.pz, 1.0);
//     gl_Position = shadow_cascades.data.view_proj[gl_ViewIndex] * mat.model * positionOS;
// }