#version 460

layout(location = 0) in vec4 position_os;
layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform samplerCube cubemap;

void main()
{
    out_color = texture(cubemap, normalize(position_os.xyz));
}