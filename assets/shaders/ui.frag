#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/structs.h"
#include "lib/gamma.glsl"

// keep in sync with UIElementFlags
#define UI_FLAG_APPLY_TONEMAPPING (1 << 2)

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
    if ( element._pad == 0 )
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
    else
    {
        vec4 tint = vec4( 0, 25, 160, 255 ) / 255.0f;
        vec4 texSample = vec4( 0, 0, 0, 0 );
        if ( element.textureIndex != PG_INVALID_TEXTURE_INDEX )
	    {
		    texSample = texture( textures[element.textureIndex], texCoord );
	    }

        //vec4 tevin_a = vec4( tint.rgb, 0 );
	    //vec4 tevin_b = vec4( textemp.rgb, textemp.a );
	    //vec4 tevin_c = vec4( vertColor.rgb, tint.a );
	    //vec4 tevin_d = vec4( tint.rgb, 0 );

        //finalColor.rgb = clamp( tevin_d.rgb + tevin_a.rgb + (tevin_b.rgb - tevin_a.rgb)*tevin_c.rgb), vec3( 0 ), vec3( 1 ) );
        //finalColor.a = clamp( tevin_d.a + tevin_a.a + (tevin_b.a - tevin_a.a)*tevin_c.a), 0, 1 );
        //finalColor = tevin_d + mix( tevin_a, texSample, tevin_c );
        //finalColor = clamp( finalColor, vec4( 0 ), vec4( 1 ) );

        finalColor.rgb = saturate( tint.rgb + mix( tint.rgb, texSample.rgb, vertColor.rgb ) );
        finalColor.a = tint.a * texSample.a;
    }
}