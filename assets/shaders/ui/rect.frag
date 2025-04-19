#version 450

#include "global_descriptors.glsl"
#include "c_shared/clay_ui.h"

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIRectElementData drawData;
};

layout( location = 0 ) in PerVertexData
{
    vec2 uv;
} fragInput;

layout( location = 0 ) out vec4 finalColor;

void main()
{
    vec4 color = drawData.color;
    if ( drawData.textureIndex != PG_INVALID_TEXTURE_INDEX )
    {
        color *= SampleTexture2D( drawData.textureIndex, SAMPLER_BILINEAR, fragInput.uv );
    }
	finalColor = color;
}