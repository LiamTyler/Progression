#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"

layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};

layout( push_constant ) uniform PushConsts
{
    mat4 M;
};

layout( location = 0 ) in vec3 inPosition;

void main()
{
    vec4 worldSpacePos = M * vec4( inPosition, 1 );
    gl_Position = globals.VP * worldSpacePos;
}