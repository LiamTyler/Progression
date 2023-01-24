#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"

layout( location = 0 ) in vec2 texCoord;

//layout( set = 0, binding = 0 ) uniform sampler2D postProcessOutput;
layout( set = PG_BINDLESS_TEXTURE_SET, binding = 0 ) uniform sampler2D textures[];

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIElementData element;
};

layout( location = 0 ) out vec4 finalColor;

void main()
{
	vec4 tintColor = unpackUnorm4x8( element.packedTint );
    finalColor.rgb = tintColor.rgb;
    finalColor.a = tintColor.a;
}