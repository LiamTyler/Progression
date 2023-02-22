#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/structs.h"
#include "lib/gamma.glsl"

// keep in sync with UIElementFlags
#define UI_FLAG_APPLY_TONEMAPPING (1 << 2)

layout( location = 0 ) in vec2 texCoord;

layout( set = PG_BINDLESS_TEXTURE_SET, binding = 0 ) uniform sampler2D textures[];

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIElementData element;
};

layout( location = 0 ) out vec4 finalColor;

void main()
{
	vec4 color = unpackUnorm4x8( element.packedTint );
    if ( element.textureIndex != PG_INVALID_TEXTURE_INDEX )
	{
		color.rgba *= texture( textures[element.textureIndex], texCoord );
	}
    
    finalColor.rgb = color.rgb;
    finalColor.a = color.a;
    
    if ( 0 != (element.flags & UI_FLAG_APPLY_TONEMAPPING) )
    {
        finalColor.rgb = LinearToGammaSRGB( finalColor.rgb );
    }
}