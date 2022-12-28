//
#version 460

#extension GL_EXT_nonuniform_qualifier : require // Descriptor indexing

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;
layout(binding = 3) uniform sampler2D textures[];
layout(push_constant) uniform pushBlock { uint index; } pushConstants;
layout(location = 0) out vec4 outColor;

const uint depthTextureMask = 0xFFFF;

vec4 getColor(uint texType, vec4 value)
{
    switch (texType)
    {
        case 0:
            return color * value;
        case 1:
            return vec4(value.rrr, 1.0);
        case 2:
            return vec4(value.xyz, 1.0);
    }

    return vec4(value.xyz, 1.0);
}

void main()
{
	uint tex = pushConstants.index & depthTextureMask;
	uint texType = (pushConstants.index >> 16) & depthTextureMask;
	vec4 value = texture(textures[nonuniformEXT(tex)], uv);

	outColor = getColor(texType, value);

	//outColor = color * texture(textures[nonuniformEXT(pushConstants.textureID], uv);
}
