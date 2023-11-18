#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/dvar_defines.h"
#include "c_shared/structs.h"
#include "lib/pbr.glsl"
#include "lib/lights.glsl"

layout( location = 0 ) in vec3 worldSpacePos;
layout( location = 1 ) in vec3 worldSpaceNormal;
layout( location = 2 ) in vec3 worldSpaceTangent;
layout( location = 3 ) in vec3 worldSpaceBitangent;
layout( location = 4 ) in vec2 texCoords;

layout( location = 0 ) out vec4 outColor;


layout( set = PG_SCENE_GLOBALS_DESCRIPTOR_SET, binding = PG_SCENE_CONSTS_BINDING_SLOT ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};

layout( set = PG_BINDLESS_TEXTURE_DESCRIPTOR_SET, binding = 0 ) uniform sampler2D textures_2D[];

layout( std430, set = PG_LIGHT_DESCRIPTOR_SET, binding = 0 ) readonly buffer PackedLightSSBO
{
    PackedLight lights[];
};
layout( set = PG_LIGHTING_AUX_DESCRIPTOR_SET, binding = 0 ) uniform samplerCube skyboxIrradiance;
layout( set = PG_LIGHTING_AUX_DESCRIPTOR_SET, binding = 1 ) uniform samplerCube skyboxReflectionProbe;
layout( set = PG_LIGHTING_AUX_DESCRIPTOR_SET, binding = 2 ) uniform sampler2D brdfLUT;

layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = 128 ) MaterialData material;
};


// Note: keep in sync with gfx_image.cpp::MIP_LEVELS - 1
#define MAX_REFLECTION_LOD 7 
#define DIELECTRIC_SPECULAR 0.04f


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

vec3 GetEmissive( const vec2 uv )
{
    vec3 emissive = material.emissiveTint;
    if ( material.emissiveMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec3 emissiveSample = texture( textures_2D[material.emissiveMapIndex], uv ).rgb;
        emissive *= emissiveSample;
    }

    return emissive;
}

struct Material
{
    vec3 albedo;
    float metalness;
    vec3 N;
    float roughness;
    vec3 emissive;
    vec3 F0;
};

Material GetMaterial( const vec2 uv )
{
    Material m;
    GetAlbedoMetalness( uv, m.albedo, m.metalness );
    GetNormalRoughness( uv, m.N, m.roughness );
    m.emissive = GetEmissive( uv );
    m.F0 = mix( vec3( DIELECTRIC_SPECULAR ), m.albedo, m.metalness );

    return m;
}

void Debug_Material( const Material m, inout vec4 outColor )
{
    uint r_materialViz = globals.r_materialViz;
    if ( r_materialViz == PG_DEBUG_MTL_ALBEDO )
    {
        outColor.rgb = m.albedo;
    }
    else if ( r_materialViz == PG_DEBUG_MTL_NORMAL )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( m.N );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_ROUGHNESS )
    {
        outColor.rgb = vec3( m.roughness );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_METALNESS )
    {
        outColor.rgb = vec3( m.metalness );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_EMISSIVE )
    {
        outColor.rgb = vec3( m.emissive );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_GEOM_NORMAL )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( normalize( worldSpaceNormal ) );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_GEOM_TANGENT )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( normalize( worldSpaceTangent ) );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_GEOM_BITANGENT )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( normalize( worldSpaceBitangent ) );
    }
}


struct LightingResults
{
    vec3 direct;
    vec3 diffuseIndirect;
    vec3 specularIndirect;
    vec3 emissive;
};

void DirectLighting( vec3 pos, const Material m, vec3 V, float NdotV, inout LightingResults lighting )
{
    vec3 diffuse = m.albedo / PI;
    float remappedRoughness = PBR_GetRemappedRoughness_Direct( m.roughness );
    vec3 Lo = vec3( 0 );

    const uint numLights = globals.lightCountAndPad3.x;
    for ( uint lightIdx = 0; lightIdx < numLights; ++lightIdx )
    {
        const PackedLight packedL = lights[lightIdx];
        vec3 radiance = packedL.colorAndType.rgb;
        const uint lightType = UnpackLightType( packedL );
        vec3 L;
        switch ( lightType )
        {
            case LIGHT_TYPE_POINT:
            {
                PointLight l = UnpackPointLight( packedL );
                radiance *= PointLightAttenuation( pos, l );
                L = normalize( l.position - pos );
                break;
            }
            case LIGHT_TYPE_SPOT:
            {
                SpotLight l = UnpackSpotLight( packedL );
                radiance *= SpotLightAttenuation( pos, l );
                L = normalize( l.position - pos );
                break;
            }
            case LIGHT_TYPE_DIRECTIONAL:
            {
                DirectionalLight l = UnpackDirectionalLight( packedL );
                radiance = l.color;
                L = -l.direction;
                break;
            }
        }

        vec3 H = normalize( V + L );
        float NdotH = max( dot( m.N, H ), 0.0f );
        float NdotL = max( dot( m.N, L ), 0.0f );
        float VdotH = max( dot( V, H ), 0.0f );

        float D = PBR_D_GGX( NdotH, m.roughness );
        float V = V_SmithGGXCorrelated( NdotV, NdotL, m.roughness );
        vec3 F = PBR_FresnelSchlick( VdotH, m.F0 );
        vec3 specular = D * F * V;

        vec3 kD = (1.0f - F) * (1.0f - m.metalness);
        Lo += (kD * diffuse + specular) * radiance * NdotL;
    }

    lighting.direct = Lo;
}

void IndirectLighting( const Material m, vec3 V, float NdotV, inout LightingResults lighting )
{
    vec3 skyTint = globals.cameraExposureAndSkyTint.yzw;
    vec3 R = reflect( -V, m.N );

    vec2 f_ScaleAndBias = texture( brdfLUT, vec2( NdotV, m.roughness ) ).rg;
    vec3 prefilteredRadiance = textureLod( skyboxReflectionProbe, R,  m.roughness * MAX_REFLECTION_LOD ).rgb * skyTint;
    vec3 irradiance = texture( skyboxIrradiance, m.N ).rgb * skyTint;

    vec3 Fr = max( vec3( 1.0f - m.roughness ), m.F0 ) - m.F0;
    vec3 kS = m.F0 + Fr * pow( 1.0f - NdotV, 5.0f );
    vec3 FssEss = kS * f_ScaleAndBias.x + f_ScaleAndBias.y;

    float Ess = f_ScaleAndBias.x + f_ScaleAndBias.y;
    float Ems = 1.0f - Ess;
    vec3 Favg = m.F0 + (1.0f - m.F0) / 21.0f;
    vec3 Fms = FssEss * Favg / (1.0f - Ems * Favg);
    vec3 FmsEms = Fms * Ems;

    vec3 Edss = 1.0f - (FssEss + FmsEms);
    vec3 kD = m.albedo * Edss;

    lighting.specularIndirect = FssEss * prefilteredRadiance;
    lighting.diffuseIndirect = (FmsEms + kD) * irradiance;
}

void EmissiveLighting( const Material m, inout LightingResults lighting )
{
    lighting.emissive = m.emissive;
}

void Debug_Lighting( const LightingResults lighting, inout vec4 outColor )
{
    uint r_lightingViz = globals.r_lightingViz;
    if ( r_lightingViz == PG_DEBUG_LIGHTING_DIRECT )
    {
        outColor.rgb = lighting.direct;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_INDIRECT )
    {
        outColor.rgb = lighting.diffuseIndirect + lighting.specularIndirect;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_DIFFUSE_INDIRECT )
    {
        outColor.rgb = lighting.diffuseIndirect;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_SPECULAR_INDIRECT )
    {
        outColor.rgb = lighting.specularIndirect;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_EMISSIVE )
    {
        outColor.rgb = lighting.emissive;
    }
}


void main()
{
    Material m = GetMaterial( texCoords );

    vec3 V = normalize( globals.cameraPos.xyz - worldSpacePos );
    float NdotV = clamp( dot( m.N, V ), 1e-5f, 1.0f );

    LightingResults lighting;
    DirectLighting( worldSpacePos, m, V, NdotV, lighting );
    IndirectLighting( m, V, NdotV, lighting );
    EmissiveLighting( m, lighting );

    vec3 Lo = vec3( 0 );
    Lo += lighting.direct;
    Lo += lighting.diffuseIndirect;
    Lo += lighting.specularIndirect;
    Lo += lighting.emissive;

    outColor = vec4( Lo, 1 );
    Debug_Material( m, outColor );
    Debug_Lighting( lighting, outColor );
}