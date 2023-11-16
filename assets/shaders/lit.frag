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

    if ( false && material.normalRoughnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 normalRoughnessSample = texture( textures_2D[material.normalRoughnessMapIndex], uv );
        vec3 nmv = UnpackNormalMapVal( normalRoughnessSample.rgb );
        vec3 T = normalize( worldSpaceTangent );
        vec3 B = normalize( worldSpaceBitangent );
        N = normalize( T * nmv.x + B * nmv.y + N * nmv.z );

        roughness *= normalRoughnessSample.a;
    }
}


vec3 DirectLighting( vec3 pos, vec3 albedo, float metalness, vec3 N, float roughness, vec3 V, vec3 F0, float NdotV )
{
    vec3 diffuse = albedo / PI;
    float remappedRoughness = PBR_GetRemappedRoughness_Direct( roughness );
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
        float NdotH = max( dot( N, H ), 0.0f );
        float NdotL = max( dot( N, L ), 0.0f );
        float VdotH = max( dot( V, H ), 0.0f );

        float D = PBR_D_GGX( NdotH, roughness );
        float V = V_SmithGGXCorrelated( NdotV, NdotL, roughness );
        vec3 F = PBR_FresnelSchlick( VdotH, F0 );
        vec3 specular = D * F * V;

        vec3 kD = (1.0f - F) * (1.0f - metalness);
        Lo += (kD * diffuse + specular) * radiance * NdotL;
    }

    return Lo;
}


void main()
{
    vec3 albedo;
    float metalness;
    GetAlbedoMetalness( texCoords, albedo, metalness );

    vec3 N;
    float roughness;
    GetNormalRoughness( texCoords, N, roughness );

    vec3 V = normalize( globals.cameraPos.xyz - worldSpacePos );
    vec3 F0 = mix( vec3( DIELECTRIC_SPECULAR ), albedo, metalness );
    float NdotV = clamp( dot( N, V ), 1e-5f, 1.0 );
    vec3 R = reflect( -V, N );
    vec3 skyTint = globals.cameraExposureAndSkyTint.yzw;

    vec3 Lo = vec3( 0 );
    vec3 directLighting = DirectLighting( worldSpacePos, albedo, metalness, N, roughness, V, F0, NdotV );
    Lo += directLighting;

    vec2 f_ScaleAndBias = texture( brdfLUT, vec2( NdotV, roughness ) ).rg;
    vec3 prefilteredRadiance = textureLod( skyboxReflectionProbe, R,  roughness * MAX_REFLECTION_LOD ).rgb * skyTint;
    vec3 irradiance = texture( skyboxIrradiance, N ).rgb * skyTint;

    vec3 specularIndirect, diffuseIndirect;

    if ( globals.debugInt == 0 )
    {
        vec3 kS = F0;
        specularIndirect = prefilteredRadiance * (kS * f_ScaleAndBias.x + f_ScaleAndBias.y);
        vec3 kD = (1.0f - kS) * (1.0f - metalness);
        diffuseIndirect = irradiance * albedo * kD; // note: not using (albedo/pi) because the irradiance map already has the pi term pre-divided
    }
    else if ( globals.debugInt == 1 )
    {
        vec3 kS = PBR_FresnelSchlickRoughness( NdotV, F0, roughness );
        specularIndirect = prefilteredRadiance * (kS * f_ScaleAndBias.x + f_ScaleAndBias.y);
        vec3 kD = (1.0f - kS) * (1.0f - metalness);
        diffuseIndirect = irradiance * albedo * kD; // note: not using (albedo/pi) because the irradiance map already has the pi term pre-divided
    }
    // https://github.com/BruOp/bae/blob/master/examples/04-pbr-ibl/fs_pbr_ibl.sc
    else if ( globals.debugInt == 2 )
    {
        vec3 kS = F0;
        vec3 FssEss = kS * f_ScaleAndBias.x + f_ScaleAndBias.y;

        float Ems = (1.0f - (f_ScaleAndBias.x + f_ScaleAndBias.y));
        vec3 Favg = F0 + (1.0f - F0) / 21.0f;
        vec3 FmsEms = Ems * FssEss * Favg / (1.0f - Favg * Ems);
        vec3 diffuseColor = albedo * (1.0f - DIELECTRIC_SPECULAR) * (1.0f - metalness);
        vec3 kD = diffuseColor * (1.0f - FssEss - FmsEms);

        specularIndirect = FssEss * prefilteredRadiance;
        diffuseIndirect = (FmsEms + kD) * irradiance;
    }
    else if ( globals.debugInt == 3 )
    {
        // Roughness dependent fresnel, from Fdez-Aguera
        vec3 Fr = max( vec3( 1.0f - roughness ), F0 ) - F0;
        vec3 kS = F0 + Fr * pow( 1.0f - NdotV, 5.0f );
        vec3 FssEss = kS * f_ScaleAndBias.x + f_ScaleAndBias.y;

        float Ems = (1.0f - (f_ScaleAndBias.x + f_ScaleAndBias.y));
        vec3 Favg = F0 + (1.0f - F0) / 21.0f;
        vec3 FmsEms = Ems * FssEss * Favg / (1.0f - Favg * Ems);
        vec3 diffuseColor = albedo;// * (1.0f - DIELECTRIC_SPECULAR) * (1.0f - metalness);
        vec3 kD = diffuseColor * (1.0f - FssEss - FmsEms);

        specularIndirect = FssEss * prefilteredRadiance;
        diffuseIndirect = (FmsEms + kD) * irradiance;
    }
    else if ( globals.debugInt == 4 )
    {
        // Roughness dependent fresnel, from Fdez-Aguera
        vec3 Fr = max( vec3( 1.0f - roughness ), F0 ) - F0;
        vec3 kS = F0 + Fr * pow( 1.0f - NdotV, 5.0f );
        vec3 FssEss = kS * f_ScaleAndBias.x + f_ScaleAndBias.y;

        float Ess = f_ScaleAndBias.x + f_ScaleAndBias.y;
        float Ems = 1.0f - Ess;
        vec3 Favg = F0 + (1.0f - F0) / 21.0f;
        vec3 Fms = FssEss * Favg / (1.0f - Ems * Favg);

        vec3 Edss = 1.0f - (FssEss + Fms * Ems);
        vec3 kD = albedo * Edss;

        specularIndirect = FssEss * prefilteredRadiance;
        diffuseIndirect = (Fms * Ems + kD) * irradiance;
    }

    Lo += diffuseIndirect;
    Lo += specularIndirect;


    // TODO: emissive

    outColor = vec4( Lo, 1 );

    uint r_lightingViz = globals.r_lightingViz;
    if ( r_lightingViz == PG_DEBUG_LIGHTING_DIRECT )
    {
        outColor.rgb = directLighting;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_INDIRECT )
    {
        outColor.rgb = diffuseIndirect + specularIndirect;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_DIFFUSE_INDIRECT )
    {
        outColor.rgb = diffuseIndirect;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_SPECULAR_INDIRECT )
    {
        outColor.rgb = specularIndirect;
    }

    uint r_materialViz = globals.r_materialViz;
    if ( r_materialViz == PG_DEBUG_MTL_ALBEDO )
    {
        outColor.rgb = albedo;
    }
    else if ( r_materialViz == PG_DEBUG_MTL_NORMAL )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( N );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_ROUGHNESS )
    {
        outColor.rgb = vec3( roughness );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_METALNESS )
    {
        outColor.rgb = vec3( metalness );
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