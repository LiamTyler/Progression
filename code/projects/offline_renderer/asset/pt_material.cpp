#include "pt_material.hpp"
#include "asset/types/material.hpp"
#include "intersection_tests.hpp"
#include "pt_image.hpp"
#include "pt_math.hpp"
#include "renderer/brdf_functions.hpp"
#include "sampling.hpp"
#include "shared/assert.hpp"
#include "shared/color_spaces.hpp"
#include "shared/random.hpp"
#include <algorithm>
#include <unordered_map>

using namespace PG;

namespace PT
{

vec3 BRDF::F( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const
{
    const vec3& L = worldSpace_wi;
    const vec3& V = worldSpace_wo;

    vec3 H      = Normalize( worldSpace_wo + worldSpace_wi );
    float NdotV = Max( Dot( N, V ), 1e-5f );
    float NdotH = Saturate( Dot( N, H ) );
    float NdotL = Saturate( Dot( N, L ) );
    float VdotH = Saturate( Dot( V, H ) );
    vec3 F0     = mix( vec3( 0.04f ), albedo, metalness );

    float linearRoughness = roughness * roughness;
    float D               = GGX_D( NdotH, linearRoughness );
    float Vis             = V_SmithGGXCorrelated( NdotV, NdotL, linearRoughness );
    vec3 F                = FresnelSchlick( VdotH, F0 );
    vec3 specular         = D * F * Vis;

    vec3 kD = ( vec3( 1.0f ) - F ) * ( 1.0f - metalness );
    return ( kD * albedo / PI + specular );

    // return albedo / PI;
}

vec3 BRDF::Sample_F( const vec3& worldSpace_wo, vec3& worldSpace_wi, PG::Random::RNG& rng, float& pdf ) const
{
    float u = rng.UniformFloat();
    float v = rng.UniformFloat();
    // vec3 localWi  = CosineSampleHemisphere( u, v );
    vec3 localWi  = UniformSampleHemisphere( u, v );
    worldSpace_wi = T * localWi.x + B * localWi.y + N * localWi.z;
    // worldSpace_wi     = T * localWi.x + B * localWi.y + N * Max( 0.00001f, localWi.z );
    pdf = Pdf( worldSpace_wo, worldSpace_wi );
    return F( worldSpace_wo, worldSpace_wi );
}

float BRDF::Pdf( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const
{
    bool sameHemisphere = Dot( worldSpace_wo, N ) * Dot( worldSpace_wi, N ) > 0;
    // return sameHemisphere ? AbsDot( worldSpace_wi, N ) / PI : 0;
    return sameHemisphere ? 1.0f / ( 2.0f * PI ) : 0;
}

void Material::GetAlbedoMetalness( const vec2& uv, vec3& outAlbedo, float& outMetalness ) const
{
    outAlbedo    = albedoTint;
    outMetalness = metalnessTint;

    if ( albedoMetalnessTex != TEXTURE_HANDLE_INVALID )
    {
        vec4 sample = GetTex( albedoMetalnessTex )->Sample( uv );
        outAlbedo *= vec3( sample );
        outMetalness *= sample.a;
    }
}

static vec3 UnpackNormalMapVal( vec3 v ) { return ( v * 255.0f - vec3( 128.0f ) ) / 127.0f; }

void Material::GetNormalRoughness( const vec2 uv, IntersectionData* surfaceInfo, vec3& outShadingN, float& outRoughness ) const
{
    outShadingN  = surfaceInfo->normal;
    outRoughness = roughnessTint;

    if ( normalRoughnessTex != TEXTURE_HANDLE_INVALID )
    {
        vec4 sample = GetTex( normalRoughnessTex )->Sample( uv );
        vec3 nmv    = UnpackNormalMapVal( vec3( sample ) );
        vec3 T      = surfaceInfo->tangent;
        vec3 B      = surfaceInfo->bitangent;
        outShadingN = Normalize( T * nmv.x + B * nmv.y + surfaceInfo->normal * nmv.z );

        outRoughness *= sample.a;
    }
}

vec3 Material::GetEmissive( const vec2& uv ) const
{
    vec3 color = emissiveTint;
    if ( emissiveTex != TEXTURE_HANDLE_INVALID )
    {
        vec4 sample = GetTex( emissiveTex )->Sample( uv );
        color *= vec3( sample );
    }

    return color;
}

BRDF Material::ComputeBRDF( IntersectionData* surfaceInfo ) const
{
    BRDF brdf;
    GetAlbedoMetalness( surfaceInfo->texCoords, brdf.albedo, brdf.metalness );
    GetNormalRoughness( surfaceInfo->texCoords, surfaceInfo, brdf.shadingNormal, brdf.roughness );
    brdf.emissive = GetEmissive( surfaceInfo->texCoords );
    brdf.T        = surfaceInfo->tangent;
    brdf.B        = surfaceInfo->bitangent;
    brdf.N        = surfaceInfo->normal;

    return brdf;
}

std::vector<Material> g_materials = { Material() };
std::unordered_map<std::string, TextureHandle> s_materialNameToHandleMap;

MaterialHandle LoadMaterialFromPGMaterial( PG::Material* material )
{
    PG_ASSERT( material && material->name.length() > 0 );
    auto it = s_materialNameToHandleMap.find( material->name );
    if ( it != s_materialNameToHandleMap.end() )
    {
        return it->second;
    }

    Material mat;
    mat.albedoTint         = material->albedoTint;
    mat.metalnessTint      = material->metalnessTint;
    mat.albedoMetalnessTex = TEXTURE_HANDLE_INVALID;
    if ( material->albedoMetalnessImage )
    {
        mat.albedoMetalnessTex = LoadTextureFromGfxImage( material->albedoMetalnessImage );
    }
    mat.roughnessTint      = material->roughnessTint;
    mat.normalRoughnessTex = TEXTURE_HANDLE_INVALID;
    if ( material->normalRoughnessImage )
    {
        mat.normalRoughnessTex = LoadTextureFromGfxImage( material->normalRoughnessImage );
    }

    mat.emissiveTint = material->emissiveTint;
    mat.emissiveTex  = TEXTURE_HANDLE_INVALID;
    if ( material->emissiveImage )
    {
        mat.emissiveTex = LoadTextureFromGfxImage( material->emissiveImage );
    }
    mat.isDecal = material->type == MaterialType::DECAL;

    g_materials.emplace_back( mat );
    MaterialHandle handle                     = static_cast<MaterialHandle>( g_materials.size() - 1 );
    s_materialNameToHandleMap[material->name] = handle;
    return handle;
}

Material* GetMaterial( MaterialHandle handle ) { return &g_materials[handle]; }

} // namespace PT
