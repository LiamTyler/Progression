#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"

layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};

layout( push_constant ) uniform PushConsts
{
    PerObjectData perObjectData;
};

layout( location = 0 ) in vec3 inPosition;

#if !DEPTH_ONLY_PASS
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 1 ) out vec3 outWorldSpacePos;
layout( location = 1 ) out vec3 outWorldSpaceNormal;
layout( location = 2 ) out vec2 outTexCoords;
#endif // #if !DEPTH_ONLY_PASS

void main()
{
    vec4 worldSpacePos = M * vec4( inPosition, 1 );
    gl_Position = globals.VP * worldSpacePos;
#if !DEPTH_ONLY_PASS
    outWorldSpacePos 	 = worldSpacePos.xyz;
	outWorldSpaceNormal  = (perObjectData.N * vec4( inNormal, 0 )).xyz;
    outTexCoords         = inTexCoord;
#endif // #if !DEPTH_ONLY_PASS
}