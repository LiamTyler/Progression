#version 450

#include "global_descriptors.glsl"
#include "c_shared/text.h"

layout( scalar, push_constant ) uniform PushConstants
{
    TextDrawData drawData;
};

layout( location = 0 ) in vec2 inUV;

layout( location = 0 ) out vec4 outColor;

float ScreenPxRange() { return drawData.unitRange; }

float Median( float r, float g, float b ) { return max( min( r, g ), min( max( r, g ), b ) ); }

vec4 GetFontColor() { return unpackUnorm4x8( drawData.packedColor ); }

void main()
{
    vec4 texSample = SampleTexture2D_Uniform( drawData.sdfFontAtlasTex, SAMPLER_BILINEAR, inUV );
    float sd = Median( texSample.r, texSample.g, texSample.b );
    //float sd = texSample.a;

    // const float cutoff = 0.5f;
    // const float smoothness = .1f;
    // float alpha = smoothstep( smoothness, -smoothness, cutoff - sd );
    // outColor = vec4( 1, 1, 1, alpha );

    float screenPxDistance = ScreenPxRange() * ( sd - 0.5 );
    float opacity          = clamp( screenPxDistance + 0.5, 0.0, 1.0 );
    outColor               = GetFontColor();
    outColor.a *= opacity;

    // outColor = vec4( 1, 1, 1, 1 );
}
