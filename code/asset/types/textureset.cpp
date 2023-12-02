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

std::string TexturesetCreateInfo::GetAlbedoMap( bool isApplied ) const { return isApplied ? albedoMap : "$white"; }

std::string TexturesetCreateInfo::GetMetalnessMap( bool isApplied ) const { return isApplied ? metalnessMap : "$default_metalness"; }

std::string TexturesetCreateInfo::GetEmissiveMap( bool isApplied ) const { return isApplied ? emissiveMap : ""; }

std::string TexturesetCreateInfo::GetNormalMap( bool isApplied ) const { return isApplied ? normalMap : "$default_normalmap"; }

std::string TexturesetCreateInfo::GetRoughnessMap( bool isApplied ) const { return isApplied ? roughnessMap : "$default_roughness"; }

std::string TexturesetCreateInfo::GetAlbedoMetalnessImageName( bool applyAlbedo, bool applyMetalness ) const
{
    std::string cacheName;
    std::string albedoMapName = GetAlbedoMap( applyAlbedo );
    cacheName += GetFilenameStem( albedoMapName );
    cacheName += "~";
    std::string metalnessMapName = GetMetalnessMap( applyMetalness );
    cacheName += GetFilenameStem( metalnessMapName );

    size_t hash = 0;
    HashCombine( hash, clampHorizontal );
    HashCombine( hash, clampVertical );
    HashCombine( hash, flipVertically );
    HashCombine( hash, albedoMapName );
    HashCombine( hash, metalnessMapName );
    HashCombine( hash, Underlying( metalnessSourceChannel ) );
    HashCombine( hash, metalnessScale );

    return cacheName + "~" + std::to_string( hash );
}

std::string TexturesetCreateInfo::GetNormalRoughnessImageName( bool applyNormals, bool applyRoughness ) const
{
    std::string cacheName;
    std::string normals = GetNormalMap( applyNormals );
    cacheName += GetFilenameStem( normals );
    cacheName += "~";
    std::string roughness = GetRoughnessMap( applyRoughness );
    cacheName += GetFilenameStem( roughness );

    size_t hash = 0;
    HashCombine( hash, clampHorizontal );
    HashCombine( hash, clampVertical );
    HashCombine( hash, flipVertically );
    HashCombine( hash, normals );
    HashCombine( hash, slopeScale );
    HashCombine( hash, normalMapIsYUp );
    HashCombine( hash, roughness );
    HashCombine( hash, Underlying( roughnessSourceChannel ) );
    HashCombine( hash, invertRoughness );
    HashCombine( hash, roughnessScale );

    return cacheName + "~" + std::to_string( hash );
}

} // namespace PG
