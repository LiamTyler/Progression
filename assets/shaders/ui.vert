#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"

vec2 positions[6] = vec2[](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1),
    
    vec2(1, 1),
    vec2(1, 0),
    vec2(0, 0)
);

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIElementData element;
};

layout( location = 0 ) out vec2 texCoord;

void main()
{
	vec2 pos = positions[gl_VertexIndex] + element.dimensions * element.pos;
    gl_Position = vec4( pos, 0.0, 1.0 );
    texCoord    = positions[gl_VertexIndex];
    texCoord.y = 1 - texCoord.y;
}   