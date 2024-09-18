#version 450

#include "global_descriptors.glsl"
#include "c_shared/debug_text.h"

layout( scalar, push_constant ) uniform PushConstants
{
    TextDrawData drawData;
};

layout( location = 0 ) in vec2 inUV;
layout( location = 1 ) in vec4 inColor;

layout( location = 0 ) out vec4 outColor;

void main()
{
    float v = SampleTexture2D_Uniform( drawData.fontTexture, SAMPLER_BILINEAR, inUV ).r;
    outColor = vec4( inColor.rgb, v );
}