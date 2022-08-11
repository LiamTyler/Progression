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

std::string TexturesetCreateInfo::GetAlbedoMetalnessImageName( bool applyAlbedo, bool applyMetalness ) const
{
    size_t hash = 0;
    std::string cacheName;
    if ( !applyAlbedo || albedoMap.empty() )
    {
        cacheName += "$white";
    }
    else
    {
        HashCombine( hash, albedoMap );
        cacheName += GetFilenameStem( albedoMap );
    }
    cacheName += "~";

    if ( !applyMetalness || metalnessMap.empty() )
    {
        cacheName += "$default_metalness";
    }
    else
    {
        HashCombine( hash, metalnessMap );
        cacheName += GetFilenameStem( metalnessMap );
    }
    HashCombine( hash, metalnessScale );
    HashCombine( hash, Underlying( metalnessSourceChannel ) );
    
    return cacheName + "~" + std::to_string( hash );
}


std::string TexturesetCreateInfo::GetNormalRoughImageName( bool applyNormals, bool applyRoughness ) const
{
    size_t hash = 0;
    std::string cacheName;
    if ( !applyNormals || normalMap.empty() )
    {
        cacheName += "$default_normals";
    }
    else
    {
        HashCombine( hash, normalMap );
        cacheName += GetFilenameStem( normalMap );
    }
    cacheName += "~";

    if ( !applyRoughness || roughnessMap.empty() )
    {
        cacheName += "$default_roughness";
    }
    else
    {
        HashCombine( hash, roughnessMap );
        cacheName += GetFilenameStem( roughnessMap );
    }
    HashCombine( hash, slopeScale );
    HashCombine( hash, Underlying( roughnessSourceChannel ) );
    HashCombine( hash, invertRoughness );
    
    return cacheName + "~" + std::to_string( hash );
}

} // namespace PG
