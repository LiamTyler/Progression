#version 460

#include "global_descriptors.glsl"

layout( push_constant ) uniform PushConstants
{
    FloatBuffer vertexBuffer;
} pConstants;

layout (location = 0) out vec4 lineColor;

vec4 UnpackColor( uint index )
{
    float packed = pConstants.vertexBuffer.data[nonuniformEXT(7 * index + 6)];
    return unpackUnorm4x8( floatBitsToUint( packed ) );
}

void main()
{
    uint lineIdx = gl_VertexIndex / 2;
    uint endPoint = gl_VertexIndex & 1;
    vec3 worldPos;
    worldPos.x = pConstants.vertexBuffer.data[nonuniformEXT(7 * lineIdx + 3 * endPoint + 0)];
    worldPos.y = pConstants.vertexBuffer.data[nonuniformEXT(7 * lineIdx + 3 * endPoint + 1)];
    worldPos.z = pConstants.vertexBuffer.data[nonuniformEXT(7 * lineIdx + 3 * endPoint + 2)];
    
    lineColor = UnpackColor( lineIdx );
    gl_Position = globals.VP * vec4( worldPos, 1 );
}