#include "pt_material.hpp"
#include "asset/types/material.hpp"
#include "intersection_tests.hpp"
#include "pt_image.hpp"
#include "sampling.hpp"
#include "shared/assert.hpp"
#include "shared/color_spaces.hpp"
#include "shared/random.hpp"
#include <algorithm>
#include <unordered_map>

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


glm::vec3 BRDF::Sample_F( const glm::vec3& worldSpace_wo, glm::vec3& worldSpace_wi, PG::Random::RNG& rng, float& pdf ) const
{
    float u = rng.UniformFloat();
    float v = rng.UniformFloat();
    glm::vec3 localWi = CosineSampleHemisphere( u, v );
    worldSpace_wi     = T * localWi.x + B * localWi.y + N * localWi.z;
    //worldSpace_wi     = T * localWi.x + B * localWi.y + N * std::max( 0.00001f, localWi.z );
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
    if ( albedoTex != TEXTURE_HANDLE_INVALID )
    {
        glm::vec4 sample = GetTex( albedoTex )->Sample( texCoords );
        color *= glm::vec3( sample );
    }

    return color;
}


glm::vec3 Material::GetEmissive( const glm::vec2& texCoords ) const
{
    glm::vec3 color = emissiveTint;
    if ( emissiveTex != TEXTURE_HANDLE_INVALID )
    {
        glm::vec4 sample = GetTex( emissiveTex )->Sample( texCoords );
        color *= glm::vec3( sample );
    }

    return color;
}


BRDF Material::ComputeBRDF( IntersectionData* surfaceInfo ) const
{
    BRDF brdf;
    brdf.Kd = GetAlbedo( surfaceInfo->texCoords );
    brdf.Ke = GetEmissive( surfaceInfo->texCoords );
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
    mat.albedoTex = TEXTURE_HANDLE_INVALID;
    if ( material->albedoMetalnessImage )
    {
        mat.albedoTex = LoadTextureFromGfxImage( material->albedoMetalnessImage );
    }
    mat.emissiveTint = material->emissiveTint;
    mat.emissiveTex = TEXTURE_HANDLE_INVALID;
    if ( material->emissiveImage )
    {
        mat.emissiveTex = LoadTextureFromGfxImage( material->emissiveImage );
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