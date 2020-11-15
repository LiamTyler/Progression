#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 worldSpacePos;
layout( location = 1 ) in vec3 worldSpaceNormal;
layout( location = 2 ) in vec2 texCoords;

layout( location = 0 ) out vec4 outColor;


layout( set = 1, binding = 0 ) uniform sampler2D albedoTex;

void main()
{
    vec3 albedo = texture( albedoTex, vec2( texCoords.x, texCoords.y) ).rgb;
    //vec3 albedo = vec3( 1 - texCoords, 0 );
    outColor = vec4( albedo, 1 );
}