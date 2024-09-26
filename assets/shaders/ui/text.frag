#version 450

#include "global_descriptors.glsl"
#include "c_shared/text.h"

layout( scalar, push_constant ) uniform PushConstants
{
    TextDrawData drawData;
};

layout( location = 0 ) in vec2 inUV;

layout( location = 0 ) out vec4 outColor;

float screenPxRange()
{
    return 12.0f / 32.0f * 4;
}

void main()
{
    const float cutoff = 0.5f;
    float sd = SampleTexture2D_Uniform( drawData.sdfFontAtlasTex, SAMPLER_BILINEAR, inUV ).r;

    //const float smoothness = .1f;
    //float alpha = smoothstep( smoothness, -smoothness, cutoff - sd );
    //outColor = vec4( 1, 1, 1, alpha );

    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp( screenPxDistance + 0.5, 0.0, 1.0 );
    outColor = vec4( 1, 1, 1, opacity );
    
    
    
    //outColor = vec4( 1, 1, 1, 1 );
}