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

void GetAlbedoMetalness( const vec2 uv, out vec3 albedo, out float metalness )
{
    albedo = material.albedoTint.rgb;
    metalness = material.metalnessTint;

    if ( material.albedoMetalnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 albedoMetalnessSample = texture( textures_2D[material.albedoMetalnessMapIndex], uv );
        albedo *= albedoMetalnessSample.rgb;
        metalness *= albedoMetalnessSample.a;
    }
}

void GetNormalRoughness( const vec2 uv, out vec3 N, out float roughness )
{
    N = normalize( worldSpaceNormal );
    roughness = material.roughnessTint;

    if ( material.normalRoughnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 normalRoughnessSample = texture( textures_2D[material.normalRoughnessMapIndex], uv );
        vec3 nmv = UnpackNormalMapVal( normalRoughnessSample.rgb );
        vec3 T = normalize( worldSpaceTangent );
        vec3 B = normalize( worldSpaceBitangent );
        N = normalize( T * nmv.x + B * nmv.y + N * nmv.z );

        roughness *= normalRoughnessSample.a;
    }
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   


void main()
{
    vec3 albedo;
    float metalness;
    GetAlbedoMetalness( texCoords, albedo, metalness );

    vec3 N;
    float roughness;
    GetNormalRoughness( texCoords, N, roughness );
    roughness = 1;

    vec3 V = normalize( globals.cameraPos.xyz - worldSpacePos );
    vec3 F0 = vec3( 0.04 ); 
    F0 = mix( F0, albedo, metalness );
    float NdotV = clamp( dot( N, V ), 0.0, 1.0 );

    vec3 Lo = vec3( 0 );
    // TODO: direct lighting

    // Diffuse IBL lighting
    vec3 irradiance = texture( skyboxIrradiance, N ).rgb;
    vec3 kS = fresnelSchlickRoughness( NdotV, F0, roughness );
    vec3 kD = (1.0 - kS) * (1.0 - metalness);
    vec3 diffuse = irradiance * (albedo / PI) * kD;
    Lo += diffuse;

    // TODO: specular IBL

    // TODO: emissive


    outColor = vec4( Lo, 1 );


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