#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/structs.h"

layout( location = 0 ) in vec3 worldSpacePos;
layout( location = 1 ) in vec3 worldSpaceNormal;
layout( location = 2 ) in vec2 texCoords;

layout( location = 0 ) out vec4 outColor;


layout( set = 1, binding = 0 ) uniform sampler2D textures[];

layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = 128 ) MaterialData material;
};

void main()
{
	vec3 albedo = material.albedoTint.rgb;
	if ( material.albedoMapIndex != PG_INVALID_TEXTURE_INDEX )
	{
		albedo *= texture( textures[material.albedoMapIndex], texCoords ).rgb;
	}
	
    outColor = vec4( albedo, 1 );
}