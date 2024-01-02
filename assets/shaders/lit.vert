#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"

layout( set = PG_SCENE_GLOBALS_DESCRIPTOR_SET, binding = 0 ) uniform SceneGlobalUBO
{
	SceneGlobals globals;
};

layout( std430, set = PG_OBJECT_DESCRIPTOR_SET, binding = 0 ) readonly buffer ObjectMatricesSSBO
{
    mat4 objectMatrices[];
};

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec4 inTangentAndSign;
layout( location = 3 ) in vec2 inTexCoord;

layout( location = 0 ) out vec3 worldSpacePos;
layout( location = 1 ) out vec3 worldSpaceNormal;
layout( location = 2 ) out vec3 worldSpaceTangent;
layout( location = 3 ) out vec3 worldSpaceBitangent;
layout( location = 4 ) out vec2 texCoords;

layout( push_constant ) uniform PushConsts
{
    PerObjectData perObjectData;
};

void main()
{
    mat4 M = objectMatrices[2 * perObjectData.matrixIndex + 0];
    mat4 N = objectMatrices[2 * perObjectData.matrixIndex + 1];
    worldSpacePos = (M * vec4( inPosition, 1 )).xyz;
    worldSpaceNormal = (N * vec4( inNormal, 0 )).xyz;
    worldSpaceTangent = (M * vec4( inTangentAndSign.xyz, 0 )).xyz;
    worldSpaceBitangent = cross( worldSpaceNormal, worldSpaceTangent ) * inTangentAndSign.w;
    texCoords = inTexCoord;
    gl_Position = globals.VP * vec4( worldSpacePos, 1.0 );
}