#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout( location = 0 ) in vec3 worldSpacePos;
layout( location = 1 ) in vec3 worldSpaceNormal;
layout( location = 2 ) in vec2 texCoords;

layout( location = 0 ) out vec4 outColor;


//layout( set = 1, binding = 0 ) uniform sampler2D albedoTex;
layout( set = 1, binding = 0 ) uniform sampler2D textures[];

layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = 128 ) uint textureIndex;
};

void main()
{
    //vec3 albedo = texture( albedoTex, vec2( texCoords.x, texCoords.y) ).rgb;
    vec3 albedo = texture( textures[textureIndex], vec2( texCoords.x, texCoords.y) ).rgb;
    outColor = vec4( albedo, 1 );
}