#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "c_shared/structs.h"

layout( location = 0 ) in vec3 UV;

//layout( set = 0, binding = 0 ) uniform samplerCube skybox;
layout( set = 0, binding = 0 ) uniform sampler2D skybox;
layout( set = 0, binding = 1 ) uniform samplerCube skyboxIrradiance;

layout( location = 0 ) out vec4 finalColor;

layout( std430, push_constant ) uniform SkyboxDataUniform
{
    layout( offset = 64 ) SkyboxData drawData;
};

void main()
{
    if ( drawData.hasTexture != 0 )
    {
        vec3 dir = normalize( UV );

        // NOTE: Keep in sync with image.cpp::DirectionToEquirectangularUV
        float lon = atan( dir.x, dir.y ); // -pi to pi
        float lat = acos( dir.z ); // 0 to PI
        vec2 uv = vec2( 0.5f * lon / PI + 0.5f, lat / PI );

        if ( (drawData.debug & 1) == 0 )
        {
            finalColor = texture( skybox, uv );
        }
        else
        {
            finalColor = texture( skyboxIrradiance, dir );
            return;
        }
    }
    else
    {
        finalColor.rgb = vec3( 1, 1, 1 );
    }

    finalColor.rgb *= drawData.tint.rgb;
    finalColor.rgb *= drawData.scale;
    finalColor.a = 1;
}