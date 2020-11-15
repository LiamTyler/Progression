#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"

layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec3 worldSpacePos;
layout( location = 1 ) out vec3 worldSpaceNormal;
layout( location = 2 ) out vec2 texCoords;

void main()
{
	worldSpacePos 	 = inPosition;
	worldSpaceNormal = inNormal;
    texCoords        = inTexCoord;
	gl_Position 	 = globals.VP * vec4( worldSpacePos, 1.0 );
}