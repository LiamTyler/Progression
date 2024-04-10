#pragma once

#include "asset/pt_image.hpp"
#include "shared/math_vec.hpp"
#include "shared/random.hpp"

namespace PG
{
struct Material;
} // namespace PG

namespace PT
{

// Basically the same as 'Material' further below, but with the texturing and tinting taken care of
struct BRDF
{
    vec3 F( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const;
    vec3 Sample_F( const vec3& worldSpace_wo, vec3& worldSpace_wi, PG::Random::RNG& rng, float& pdf ) const;
    float Pdf( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const;

    vec3 albedo;
    float metalness;
    vec3 shadingNormal;
    float roughness;
    vec3 emissive;
    vec3 T, B, N; // geometric tangent space. Aka, before normal mapping
};

struct IntersectionData;

struct Material
{
    vec3 albedoTint                  = vec3( 1.0f );
    float metalnessTint              = 1.0f;
    float roughnessTint              = 1.0f;
    vec3 emissiveTint                = vec3( 0.0f );
    TextureHandle albedoMetalnessTex = TEXTURE_HANDLE_INVALID;
    TextureHandle normalRoughnessTex = TEXTURE_HANDLE_INVALID;
    TextureHandle emissiveTex        = TEXTURE_HANDLE_INVALID;
    bool isDecal                     = false;

    void GetAlbedoMetalness( const vec2& uv, vec3& outAlbedo, float& outMetalness ) const;
    void GetNormalRoughness( const vec2 uv, IntersectionData* surfaceInfo, vec3& outShadingN, float& outRoughness ) const;
    vec3 GetEmissive( const vec2& uv ) const;
    BRDF ComputeBRDF( IntersectionData* surfaceInfo ) const;
};

using MaterialHandle                             = uint16_t;
constexpr MaterialHandle MATERIAL_HANDLE_INVALID = 0;

MaterialHandle LoadMaterialFromPGMaterial( PG::Material* material );

Material* GetMaterial( MaterialHandle handle );

} // namespace PT
