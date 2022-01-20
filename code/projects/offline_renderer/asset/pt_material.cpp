#include "pt_material.hpp"
#include "asset/types/material.hpp"
#include "intersection_tests.hpp"
#include "pt_image.hpp"
#include "sampling.hpp"
#include "shared/assert.hpp"
#include "shared/random.hpp"
#include <algorithm>

using namespace PG;

namespace PT
{

glm::vec3 FresnelSchlick( const glm::vec3& F0, float cosTheta )
{
    return F0 + (glm::vec3( 1.0f ) - F0)*std::pow(1.0f - cosTheta, 5.0f );
}


glm::vec3 BRDF::F( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const
{
    return Kd / PI;
}


glm::vec3 BRDF::Sample_F( const glm::vec3& worldSpace_wo, glm::vec3& worldSpace_wi, float& pdf ) const
{
    glm::vec3 localWi = CosineSampleHemisphere( Random::Rand(), Random::Rand() );
    worldSpace_wi     = T * localWi.x + B * localWi.y + N * localWi.z;
    pdf               = Pdf( worldSpace_wo, worldSpace_wi );
    return F( worldSpace_wo, worldSpace_wi );
}

float BRDF::Pdf( const glm::vec3& worldSpace_wo, const glm::vec3& worldSpace_wi ) const
{
    bool sameHemisphere = glm::dot( worldSpace_wo, N ) * glm::dot( worldSpace_wi, N ) > 0;
    return sameHemisphere ? AbsDot( worldSpace_wi, N ) / PI : 0;
}


glm::vec3 Material::GetAlbedo( const glm::vec2& texCoords ) const
{
    glm::vec3 color = albedoTint;
    if ( albedoTex )
    {
        color *= glm::vec3( GetTex( albedoTex )->Sample( texCoords ) );
    }

    return color;
}


BRDF Material::ComputeBRDF( IntersectionData* surfaceInfo ) const
{
    BRDF brdf;
    brdf.Kd = GetAlbedo( surfaceInfo->texCoords );
    brdf.T  = surfaceInfo->tangent;
    brdf.B  = surfaceInfo->bitangent;
    brdf.N  = surfaceInfo->normal;

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
    mat.albedoTint = material->albedoTint;
    if ( material->albedoMap )
    {
        mat.albedoTex = LoadTextureFromGfxImage( material->albedoMap );
    }

    g_materials.emplace_back( mat );
    MaterialHandle handle = static_cast< MaterialHandle >( g_materials.size() - 1 );
    s_materialNameToHandleMap[material->name] = handle;
    return handle;
}


Material* GetMaterial( MaterialHandle handle )
{
    return &g_materials[handle];
}

} // namespace PT