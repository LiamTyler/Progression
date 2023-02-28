#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "c_shared/structs.h"

layout( location = 0 ) in vec3 UV;

//layout( set = 0, binding = 0 ) uniform samplerCube skybox;
layout( set = 0, binding = 0 ) uniform sampler2D skybox;

layout( location = 0 ) out vec4 finalColor;

layout( std430, push_constant ) uniform SkyboxDataUniform
{
    layout( offset = 64 ) SkyboxData drawData;
};

void main()
{
    if ( drawData.hasTexture != 0 )
    {
        // Flip Z because in opengl, +z is forward instead of -z, because it uses a left handed system
        // https://stackoverflow.com/questions/11685608/convention-of-faces-in-opengl-cubemapping
        //finalColor = texture( skybox, vec3( UV.xy, -UV.z ) );
        vec3 dir = normalize( vec3( UV.xy, -UV.z ) );
        float lon = atan( dir.x, dir.y );
        float lat = atan( dir.z, length( dir.xy ) );
        vec2 uv = vec2( 0.5 * (lon / PI + 1), lat / PI + 0.5 );
        finalColor = texture( skybox, uv );
    }
    else
    {
        finalColor.rgb = vec3( 1, 1, 1 );
    }

    finalColor.rgb *= drawData.tint.rgb;
    finalColor.rgb *= drawData.scale;
    finalColor.a = 1;
}