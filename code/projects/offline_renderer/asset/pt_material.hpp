#pragma once

#include "asset/pt_image.hpp"
#include "shared/math.hpp"
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
    glm::vec3 F( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const;
    glm::vec3 Sample_F( const glm::vec3& worldSpace_wo, glm::vec3& worldSpace_wi, PG::Random::RNG& rng, float& pdf ) const;
    float Pdf( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const;

    glm::vec3 Kd;
    glm::vec3 Ke;
    glm::vec3 T, B, N;
};

struct IntersectionData;

struct Material
{
    glm::vec3 albedoTint      = glm::vec3( 0.0f );
    TextureHandle albedoTex   = TEXTURE_HANDLE_INVALID;
    glm::vec3 emissiveTint    = glm::vec3( 0.0f );
    TextureHandle emissiveTex = TEXTURE_HANDLE_INVALID;

    glm::vec3 GetAlbedo( const glm::vec2& texCoords ) const;
    glm::vec3 GetEmissive( const glm::vec2& texCoords ) const;
    BRDF ComputeBRDF( IntersectionData* surfaceInfo ) const;
};

using MaterialHandle                             = uint16_t;
constexpr MaterialHandle MATERIAL_HANDLE_INVALID = 0;

MaterialHandle LoadMaterialFromPGMaterial( PG::Material* material );

Material* GetMaterial( MaterialHandle handle );

} // namespace PT
