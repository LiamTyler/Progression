#version 450

#include "common.glsl"
#include "c_shared/text.h"

layout( push_constant ) uniform PushConstants
{
    TextDrawData drawData;
};

layout( location = 0 ) out vec2 UV;

vec2 GetOffset( uint i )
{
    vec2 v;
    v.x = int( i == 2 || i == 4 || i == 5 );
    v.y = int( i == 1 || i == 2 || i == 4 );
    return v;
}

void main()
{
    uint localVIdx = gl_VertexIndex % 6;
    uint charIdx = gl_VertexIndex / 6;
    
    vec2 pos    = drawData.vertexBuffer.data[4 * charIdx + 0];
    vec2 size   = drawData.vertexBuffer.data[4 * charIdx + 1];
    vec2 uvUL   = drawData.vertexBuffer.data[4 * charIdx + 2];
    vec2 uvSize = drawData.vertexBuffer.data[4 * charIdx + 3];

    vec2 offset = GetOffset( localVIdx );
    pos = pos + size * offset;
    gl_Position = drawData.projMatrix * vec4( pos, 0, 1 );
    
    UV = uvUL + uvSize * offset;
}