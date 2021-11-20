#pragma once

#include "math.hpp"
#include "resource/resource.hpp"
#include "resource/texture.hpp"
#include <memory>

namespace PT
{

// just a lambertian BRDF so far
struct BRDF
{
    glm::vec3 F( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const;
    glm::vec3 Sample_F( const glm::vec3& worldSpace_wo, glm::vec3& worldSpace_wi, float& pdf ) const;
    float Pdf( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const;

    glm::vec3 Kd;
    glm::vec3 Ks;
    float Ns;
    glm::vec3 T, B, N;
};

struct IntersectionData;

struct Material : public Resource
{
    glm::vec3 albedo = glm::vec3( 0.0f );
    glm::vec3 Ks     = glm::vec3( 0.0f );
    glm::vec3 Ke     = glm::vec3( 0.0f );
    float Ns         = 0.0f;
    glm::vec3 Tr     = glm::vec3( 0 );
    float ior        = 1.0f;
    std::shared_ptr< Texture > albedoTexture;

    glm::vec3 GetAlbedo( const glm::vec2& texCoords ) const;
    BRDF ComputeBRDF( IntersectionData* surfaceInfo ) const;
};

} // namespace PT