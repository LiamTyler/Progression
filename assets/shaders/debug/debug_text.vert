#version 450

#include "common.glsl"
#include "c_shared/debug_text.h"

layout( push_constant ) uniform PushConstants
{
    TextDrawData drawData;
};

layout( location = 0 ) out vec2 UV;
layout( location = 1 ) out vec4 color;

void main()
{
    vec4 vertData = drawData.vertexBuffer.data[gl_VertexIndex];

    gl_Position = vec4( 2 * vertData.xy - vec2( 1 ), 0, 1 );
    UV = unpackHalf2x16( floatBitsToUint( vertData.z ) );
    color = unpackUnorm4x8( floatBitsToUint( vertData.w ) );
}