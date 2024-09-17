#version 450

#include "global_descriptors.glsl"
#include "c_shared/sky.h"

layout( push_constant ) uniform PushConstants
{
    SkyDrawData drawData;
};

layout( location = 0 ) out vec3 worldSpaceDir;

void main()
{
    vec3 localPos = drawData.vertexBuffer.data[gl_VertexIndex];
    vec4 pos = drawData.VP * vec4( localPos, 1 );
    gl_Position = vec4( pos.xy, pos.w, pos.w );
    
    worldSpaceDir = localPos;
}