#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/limits.h"
#include "c_shared/structs.h"
#include "lib/pbr.glsl"

layout( location = 0 ) in vec3 worldSpacePos;
layout( location = 1 ) in vec3 worldSpaceNormal;
layout( location = 2 ) in vec2 texCoords;

layout( location = 0 ) out vec4 outColor;


layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};

layout( set = PG_BINDLESS_TEXTURE_SET, binding = 0 ) uniform sampler2D textures[];
//layout( std140, set = PG_LIGHTS_SET, binding = PG_POINT_LIGHTS_BIND_INDEX ) uniform PointLights
//{
//    PointLight pointLights[PG_MAX_NUM_GPU_POINT_LIGHTS];
//};
layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = 128 ) MaterialData material;
};

void main()
{
    vec3 albedo = material.albedoTint.rgb;
    if ( material.albedoMetalnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        albedo *= texture( textures[material.albedoMetalnessMapIndex], texCoords ).rgb;
    }
    outColor = vec4( albedo, 1 );

    // float metalness = material.metalnessTint;
    // if ( material.metalnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    // {
    // 	metalness *= texture( textures[material.metalnessMapIndex], texCoords ).r;
    // }
    // // float roughness = material.roughnessTint;
    // // if ( material.roughnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    // // {
    // // 	roughness *= texture( textures[material.roughnessMapIndex], texCoords ).r;
    // // }
    // 
    // vec3 N = normalize( worldSpaceNormal );
    // vec3 V = normalize( globals.cameraPos.xyz - worldSpacePos );
    // 
    // vec3 F0 = vec3( 0.04 ); 
    // F0 = mix( F0, albedo, metalness );
    // 
    // // reflectance equation
    // vec3 Lo = vec3( 0.0 );
    // 
    // vec3 ambient = vec3( 0.1 ) * albedo + vec3( 0.0001 * metalness );  //+ 0.0001 * roughness );
    // vec3 color = ambient + Lo;
    // outColor = vec4( color, 1 );
}