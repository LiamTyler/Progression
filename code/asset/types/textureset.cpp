#include "asset/types/textureset.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <fstream>

namespace PG
{

std::string TexturesetCreateInfo::GetAlbedoMetalnessImageName() const
{
    std::string cacheName = albedoMap.empty() ? "$white" : GetFilenameStem( albedoMap );
    cacheName += "~" + metalnessMap.empty() ? "$default_metalness" : GetFilenameStem( metalnessMap );
    if ( metalnessScale != 1.0f )   
    {
        cacheName += "~" + std::to_string( metalnessScale );
    }
    if ( metalnessSourceChannel != ChannelSelect::R )
    {
        cacheName += "~" + std::to_string( Underlying( metalnessSourceChannel ) );
    }
    
    return cacheName;
}


std::string TexturesetCreateInfo::GetNormalRoughImageName() const
{
    std::string cacheName = normalMap.empty() ? "$default_normals" : GetFilenameStem( normalMap );
    if ( slopeScale != 1.0f )   
    {
        cacheName += "~" + std::to_string( slopeScale );
    }
    cacheName += "~" + roughnessMap.empty() ? "$default_roughness" : GetFilenameStem( roughnessMap );
    if ( roughnessSourceChannel != ChannelSelect::R )
    {
        cacheName += "~" + std::to_string( Underlying( roughnessSourceChannel ) );
    }
    if ( invertRoughness )
    {
        cacheName += "~I";
    }
    
    return cacheName;
}


bool Textureset::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const TexturesetCreateInfo* createInfo = (const TexturesetCreateInfo*)baseInfo;
    name = createInfo->name;

    return true;
}


bool Textureset::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );

    return true;
}


bool Textureset::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
    serializer->Write( name );

    return true;
}

} // namespace PG
