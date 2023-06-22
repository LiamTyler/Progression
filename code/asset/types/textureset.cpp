#include "asset/types/textureset.hpp"
#include "asset/asset_manager.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <fstream>

namespace PG
{

std::string TexturesetCreateInfo::GetAlbedoMap( bool isApplied ) const
{
    return isApplied ? albedoMap : "$white";
}

std::string TexturesetCreateInfo::GetMetalnessMap( bool isApplied ) const
{
    return isApplied ? metalnessMap : "$default_metalness";
}

std::string TexturesetCreateInfo::GetEmissiveMap( bool isApplied ) const
{
    return isApplied ? emissiveMap : "";
}

//std::string TexturesetCreateInfo::GetNormalMap( bool isApplied ) const
//{
//    return isApplied ? normalMap : "$default_normalmap";
//}
//
//std::string TexturesetCreateInfo::GetRoughnessMap( bool isApplied ) const
//{
//    return isApplied ? roughnessMap : "$default_roughness";
//}


std::string TexturesetCreateInfo::GetAlbedoMetalnessImageName( bool applyAlbedo, bool applyMetalness ) const
{
    std::string cacheName;
    std::string albedo = GetAlbedoMap( applyAlbedo );
    cacheName += GetFilenameStem( albedo );
    cacheName += "~";
    std::string metal = GetMetalnessMap( applyMetalness );
    cacheName += GetFilenameStem( metal );

    size_t hash = 0;
    HashCombine( hash, albedo );
    HashCombine( hash, metal );
    HashCombine( hash, metalnessScale );
    HashCombine( hash, Underlying( metalnessSourceChannel ) );
    
    return cacheName + "~" + std::to_string( hash );
}


//std::string TexturesetCreateInfo::GetNormalRoughImageName( bool applyNormals, bool applyRoughness ) const
//{
//    std::string cacheName;
//    std::string normals = GetAlbedoMap( applyNormals );
//    cacheName += GetFilenameStem( normals );
//    cacheName += "~";
//    std::string roughness = GetMetalnessMap( applyRoughness );
//    cacheName += GetFilenameStem( roughness );
//
//    size_t hash = 0;
//    HashCombine( hash, normals );
//    HashCombine( hash, roughness );
//    HashCombine( hash, roughnessScale );
//    HashCombine( hash, Underlying( roughnessSourceChannel ) );
//    HashCombine( hash, invertRoughness );
//    
//    return cacheName + "~" + std::to_string( hash );
//}

} // namespace PG
