#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/limits.h"
#include "c_shared/structs.h"
#include "lib/pbr.glsl"

layout( location = 0 ) in vec3 worldSpacePos;
layout( location = 1 ) in vec3 worldSpaceNormal;
layout( location = 2 ) in vec3 worldSpaceTangent;
layout( location = 3 ) in vec3 worldSpaceBitangent;
layout( location = 4 ) in vec2 texCoords;

layout( location = 0 ) out vec4 outColor;


layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};

layout( set = PG_BINDLESS_TEXTURE_SET, binding = 0 ) uniform sampler2D textures_2D[];
// layout( set = PG_BINDLESS_TEXTURE_SET, binding = 0 ) uniform samplerCube textures_Cube[];
layout( set = 3, binding = 0 ) uniform samplerCube skyboxIrradiance;


layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = 128 ) MaterialData material;
};

vec3 Vec3ToUnorm( vec3 v )
{
    return clamp( 0.5f * (v + vec3( 1 )), vec3( 0 ), vec3( 1 ) );
}

vec3 UnpackNormalMapVal( vec3 v )
{
    return (v * 255 - vec3( 128 )) / 127.0f;
}

void main()
{
    vec3 albedo = material.albedoTint.rgb;
    float metalness = material.metalnessTint;
    if ( material.albedoMetalnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 albedoMetalness = texture( textures_2D[material.albedoMetalnessMapIndex], texCoords );
        albedo *= albedoMetalness.rgb;
        metalness *= albedoMetalness.a;
    }

    vec3 N = normalize( worldSpaceNormal );

    float roughness = material.roughnessTint;
    if ( material.normalRoughnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 normalRoughness = texture( textures_2D[material.normalRoughnessMapIndex], texCoords );
        vec3 nmv = UnpackNormalMapVal( normalRoughness.rgb );
        vec3 T = normalize( worldSpaceTangent );
        vec3 B = normalize( worldSpaceBitangent );
        N = normalize( T * nmv.x + B * nmv.y + N * nmv.z );

        roughness *= normalRoughness.a;
    }

    vec3 irradiance = texture( skyboxIrradiance, worldSpaceNormal ).rgb;
    vec3 diffuse = irradiance * albedo;
    outColor = vec4( diffuse, 1 );

    uint r_materialViz = globals.r_materialViz;
    if ( r_materialViz == PG_DEBUG_LAYER_ALBEDO )
    {
        outColor.rgb = albedo;
    }
    else if ( r_materialViz == PG_DEBUG_LAYER_NORMAL )
    {
        outColor.rgb = Vec3ToUnorm( N );
    }
    else if ( r_materialViz == PG_DEBUG_LAYER_ROUGHNESS )
    {
        outColor.rgb = vec3( roughness );
    }
    else if ( r_materialViz == PG_DEBUG_LAYER_METALNESS )
    {
        outColor.rgb = vec3( metalness );
    }
    else if ( r_materialViz == PG_DEBUG_LAYER_GEOM_NORMAL )
    {
        outColor.rgb = Vec3ToUnorm( normalize( worldSpaceNormal ) );
    }
    else if ( r_materialViz == PG_DEBUG_LAYER_GEOM_TANGENT )
    {
        outColor.rgb = Vec3ToUnorm( normalize( worldSpaceTangent ) );
    }
    else if ( r_materialViz == PG_DEBUG_LAYER_GEOM_BITANGENT )
    {
        outColor.rgb = Vec3ToUnorm( normalize( worldSpaceBitangent ) );
    }
}