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

// just a lambertian BRDF so far
struct BRDF
{
    vec3 F( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const;
    vec3 Sample_F( const vec3& worldSpace_wo, vec3& worldSpace_wi, PG::Random::RNG& rng, float& pdf ) const;
    float Pdf( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const;

    vec3 Kd;
    vec3 Ke;
    vec3 T, B, N;
};

struct IntersectionData;

struct Material
{
    vec3 albedoTint           = vec3( 0.0f );
    TextureHandle albedoTex   = TEXTURE_HANDLE_INVALID;
    vec3 emissiveTint         = vec3( 0.0f );
    TextureHandle emissiveTex = TEXTURE_HANDLE_INVALID;

    vec3 GetAlbedo( const vec2& texCoords ) const;
    vec3 GetEmissive( const vec2& texCoords ) const;
    BRDF ComputeBRDF( IntersectionData* surfaceInfo ) const;
};

using MaterialHandle                             = uint16_t;
constexpr MaterialHandle MATERIAL_HANDLE_INVALID = 0;

MaterialHandle LoadMaterialFromPGMaterial( PG::Material* material );

Material* GetMaterial( MaterialHandle handle );

} // namespace PT
