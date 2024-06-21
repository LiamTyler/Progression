#version 460

#include "global_descriptors.glsl"

#define MODE_LINE 0
#define MODE_AABB 1
layout( push_constant ) uniform PushConstants
{
    FloatBuffer vertexBuffer;
    uint mode;
    uint _pad;
} pConstants;

layout (location = 0) out vec4 lineColor;

vec4 UnpackColor( uint index )
{
    float packed = pConstants.vertexBuffer.data[nonuniformEXT(7 * index + 6)];
    return unpackUnorm4x8( floatBitsToUint( packed ) );
}

void main()
{
    if ( pConstants.mode == MODE_LINE )
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
    else
    {
        uint aabbIdx = gl_InstanceIndex;
        vec3 aMin, aMax;
        aMin.x = pConstants.vertexBuffer.data[nonuniformEXT(7 * aabbIdx + 0)];
        aMin.y = pConstants.vertexBuffer.data[nonuniformEXT(7 * aabbIdx + 1)];
        aMin.z = pConstants.vertexBuffer.data[nonuniformEXT(7 * aabbIdx + 2)];
        aMax.x = pConstants.vertexBuffer.data[nonuniformEXT(7 * aabbIdx + 3)];
        aMax.y = pConstants.vertexBuffer.data[nonuniformEXT(7 * aabbIdx + 4)];
        aMax.z = pConstants.vertexBuffer.data[nonuniformEXT(7 * aabbIdx + 5)];
        
        vec3 p = aMin;
        switch ( gl_VertexIndex )
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 10:
        case 11:
        case 12:
        case 13:
        case 17:
        case 18:
        case 19:
        case 20:
            p.y = aMax.y;
        }
        
        switch( gl_VertexIndex )
        {
        case 3:
        case 4:
        case 5:
        case 6:
        case 12:
        case 13:
        case 14:
        case 15:
        case 19:
        case 20:
        case 21:
        case 22:
            p.x = aMax.x;
        }
        
        if ( gl_VertexIndex == 9 || gl_VertexIndex == 11 || gl_VertexIndex == 13 || gl_VertexIndex >= 15 )
            p.z = aMax.z;
        
        lineColor = UnpackColor( aabbIdx );
        gl_Position = globals.VP * vec4( p, 1 );
    }
}