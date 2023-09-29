#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/dvar_defines.h"
#include "c_shared/structs.h"

layout( location = 0 ) in vec3 UV;

layout( set = 0, binding = 0 ) uniform sampler2D skybox;
layout( set = 0, binding = 1 ) uniform samplerCube skyboxIrradiance;

layout( location = 0 ) out vec4 finalColor;

layout( std430, push_constant ) uniform SkyboxDataUniform
{
    layout( offset = 0 ) SkyboxData drawData;
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

        if ( drawData.r_skyboxViz == PG_DEBUG_SKY_REGULAR )
        {
            finalColor = texture( skybox, uv );
            finalColor.rgb *= drawData.tint.rgb;
            finalColor.rgb *= drawData.scale;
        }
        else if ( drawData.r_skyboxViz == PG_DEBUG_SKY_IRRADIANCE )
        {
            finalColor = texture( skyboxIrradiance, dir );
        }
    }
    else
    {
        finalColor.rgb = vec3( 1, 1, 1 );
    }

    if ( drawData.r_skyboxViz == PG_DEBUG_SKY_BLACK )
    {
        finalColor.rgb = vec3( 0 );
    }
    else if ( drawData.r_skyboxViz == PG_DEBUG_SKY_WHITE )
    {
        finalColor.rgb = vec3( 1 );
    }
    
    finalColor.a = 1;
}