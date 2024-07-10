#version 450

#include "global_descriptors.glsl"
#include "c_shared/dvar_defines.h"
#include "c_shared/sky.h"
#include "lib/panorama_sampling_utils.glsl"

layout( scalar, push_constant ) uniform PushConstants
{
    SkyDrawData drawData;
};

layout( location = 0 ) in vec3 inWorldSpaceDir;
layout( location = 0 ) out vec4 finalColor;

void main()
{
    vec3 worldSpaceDir = normalize( inWorldSpaceDir );
    if ( drawData.skyTextureIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec2 uv = DirectionToLatLong( worldSpaceDir );
        finalColor = SampleTexture2D_Uniform( drawData.skyTextureIndex, SAMPLER_BILINEAR_WRAP_U, uv );
    }
    else
    {
        finalColor = vec4( 1 );
    }
    
    finalColor.rgb *= drawData.scaleFactor;
    
#if IS_DEBUG_SHADER
    if ( drawData.r_skyboxViz == PG_DEBUG_SKY_BLACK )
    {
        finalColor = vec4( 0, 0, 0, 1 );
    }
    else if ( drawData.r_skyboxViz == PG_DEBUG_SKY_WHITE )
    {
        finalColor = vec4( 1 );
    }
#endif // #if IS_DEBUG_SHADER
}