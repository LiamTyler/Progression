#include "pt_material.hpp"
#include "asset/types/material.hpp"
#include "intersection_tests.hpp"
#include "pt_image.hpp"
#include "sampling.hpp"
#include "shared/assert.hpp"
#include "shared/color_spaces.hpp"
#include "shared/math.hpp"
#include "shared/random.hpp"
#include <algorithm>
#include <unordered_map>

using namespace PG;

namespace PT
{

vec3 FresnelSchlick( const vec3& F0, float cosTheta ) { return F0 + ( vec3( 1.0f ) - F0 ) * std::pow( 1.0f - cosTheta, 5.0f ); }

vec3 BRDF::F( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const { return Kd / PI; }

vec3 BRDF::Sample_F( const vec3& worldSpace_wo, vec3& worldSpace_wi, PG::Random::RNG& rng, float& pdf ) const
{
    float u       = rng.UniformFloat();
    float v       = rng.UniformFloat();
    vec3 localWi  = CosineSampleHemisphere( u, v );
    worldSpace_wi = T * localWi.x + B * localWi.y + N * localWi.z;
    // worldSpace_wi     = T * localWi.x + B * localWi.y + N * Max( 0.00001f, localWi.z );
    pdf = Pdf( worldSpace_wo, worldSpace_wi );
    return F( worldSpace_wo, worldSpace_wi );
}

float BRDF::Pdf( const vec3& worldSpace_wo, const vec3& worldSpace_wi ) const
{
    bool sameHemisphere = Dot( worldSpace_wo, N ) * Dot( worldSpace_wi, N ) > 0;
    return sameHemisphere ? AbsDot( worldSpace_wi, N ) / PI : 0;
}

vec3 Material::GetAlbedo( const vec2& texCoords ) const
{
    vec3 color = albedoTint;
    if ( albedoTex != TEXTURE_HANDLE_INVALID )
    {
        vec4 sample = GetTex( albedoTex )->Sample( texCoords );
        color *= vec3( sample );
    }

    return color;
}

vec3 Material::GetEmissive( const vec2& texCoords ) const
{
    vec3 color = emissiveTint;
    if ( emissiveTex != TEXTURE_HANDLE_INVALID )
    {
        vec4 sample = GetTex( emissiveTex )->Sample( texCoords );
        color *= vec3( sample );
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
    mat.albedoTex  = TEXTURE_HANDLE_INVALID;
    if ( material->albedoMetalnessImage )
    {
        mat.albedoTex = LoadTextureFromGfxImage( material->albedoMetalnessImage );
    }
    mat.emissiveTint = material->emissiveTint;
    mat.emissiveTex  = TEXTURE_HANDLE_INVALID;
    if ( material->emissiveImage )
    {
        mat.emissiveTex = LoadTextureFromGfxImage( material->emissiveImage );
    }

    g_materials.emplace_back( mat );
    MaterialHandle handle                     = static_cast<MaterialHandle>( g_materials.size() - 1 );
    s_materialNameToHandleMap[material->name] = handle;
    return handle;
}

Material* GetMaterial( MaterialHandle handle ) { return &g_materials[handle]; }

} // namespace PT
