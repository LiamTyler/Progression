#version 450

#include "common.glsl"

#include "c_shared/clay_ui.h"

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIRectElementData drawData;
};

layout( location = 0 ) out PerVertexData
{
    vec2 uv;
} v_out;

const vec2 offsets[] =
{
    vec2( 0, 0 ),
    vec2( 0, 1 ),
    vec2( 1, 1 ),
    vec2( 1, 0 ),
};

void main()
{   
    vec2 offset = offsets[gl_VertexIndex];
    v_out.uv = offset;
    
    vec2 pos = drawData.aabb.xy + drawData.aabb.zw * offset;
    gl_Position = drawData.projMatrix * vec4( pos, 0, 1 );
}