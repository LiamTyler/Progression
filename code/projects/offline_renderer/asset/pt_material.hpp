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
    vec3 Sample_F( const vec3& worldSpace_wo, vec3& worldSpace_wi, PG::Random::RNG& rng, f32& pdf ) const;
    f32 Pdf( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const;

    vec3 albedo;
    f32 metalness;
    vec3 shadingNormal;
    f32 roughness;
    vec3 emissive;
    vec3 T, B, N; // geometric tangent space. Aka, before normal mapping
};

struct IntersectionData;

struct Material
{
    vec3 albedoTint                  = vec3( 1.0f );
    f32 metalnessTint                = 1.0f;
    f32 roughnessTint                = 1.0f;
    vec3 emissiveTint                = vec3( 0.0f );
    TextureHandle albedoMetalnessTex = TEXTURE_HANDLE_INVALID;
    TextureHandle normalRoughnessTex = TEXTURE_HANDLE_INVALID;
    TextureHandle emissiveTex        = TEXTURE_HANDLE_INVALID;
    bool isDecal                     = false;

    void GetAlbedoMetalness( const vec2& uv, vec3& outAlbedo, f32& outMetalness ) const;
    void GetNormalRoughness( const vec2 uv, IntersectionData* surfaceInfo, vec3& outShadingN, f32& outRoughness ) const;
    vec3 GetEmissive( const vec2& uv ) const;
    BRDF ComputeBRDF( IntersectionData* surfaceInfo ) const;
};

using MaterialHandle                             = u16;
constexpr MaterialHandle MATERIAL_HANDLE_INVALID = 0;

MaterialHandle LoadMaterialFromPGMaterial( PG::Material* material );

Material* GetMaterial( MaterialHandle handle );

} // namespace PT
