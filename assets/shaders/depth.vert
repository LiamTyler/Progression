#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"

layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};

layout( location = 0 ) in vec3 inPosition;

void main()
{
	gl_Position = globals.VP * vec4( inPosition, 1.0 );
}