#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/structs.h"
#include "c_shared/ui.h"
#include "lib/gamma.glsl"

layout( location = 0 ) in vec2 texCoord;
layout( location = 1 ) in vec4 vertColor;

layout( set = PG_BINDLESS_TEXTURE_SET, binding = 0 ) uniform sampler2D textures[];

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIElementData element;
};

layout( location = 0 ) out vec4 finalColor;

vec3 saturate( const in vec3 v )
{
    return clamp( v, vec3( 0 ), vec3( 1 ) );
}

void main()
{
	vec4 tintColor = unpackUnorm4x8( element.packedTint );
    vec4 texSample = vec4( 1, 1, 1, 1 );
    if ( element.textureIndex != PG_INVALID_TEXTURE_INDEX )
	{
        texSample = texture( textures[element.textureIndex], texCoord );
	}
    
    if ( element.type == UI_ELEM_TYPE_DEFAULT )
    {
        finalColor = tintColor * texSample;
    }
    else if ( element.type == UI_ELEM_TYPE_MKDD_DIAG_TINT )
    {
        finalColor.rgb = saturate( tintColor.rgb + mix( tintColor.rgb, texSample.rgb, vertColor.rgb ) );
        finalColor.a = tintColor.a * texSample.a;
    }
    else
    {
        finalColor = vec4( 255, 20, 147, 255 ) / 255.0f;
        return;
    }

    if ( 0 != (element.flags & UI_FLAG_APPLY_TONEMAPPING) )
    {
        finalColor.rgb = LinearToGammaSRGB( finalColor.rgb );
    }
}