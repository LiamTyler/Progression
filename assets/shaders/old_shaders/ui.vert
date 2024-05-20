#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/ui.h"

vec2 positions[6] = vec2[](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1),
    
    vec2(1, 1),
    vec2(1, 0),
    vec2(0, 0)
);

vec4 colors[6] = vec4[](
    vec4(1, 1, 1, 1),
    vec4(0, 1, 1, 1),
    vec4(1, 1, 1, 1),
    
    vec4(1, 1, 1, 1),
    vec4(0, 1, 1, 1),
    vec4(1, 1, 1, 1)
);

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIElementData element;
};

layout( location = 0 ) out vec2 texCoord;
layout( location = 1 ) out vec4 color;

void main()
{
    vec2 pos = 2 * (positions[gl_VertexIndex] * element.dimensions + element.pos) - vec2( 1 );
    gl_Position = vec4( pos, 0.0, 1.0 );
    texCoord = positions[gl_VertexIndex];
    color = colors[gl_VertexIndex];
}   