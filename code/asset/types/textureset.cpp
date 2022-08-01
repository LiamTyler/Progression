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

std::string TexturesetCreateInfo::GetAlbedoMetalnessImageName() const
{
    size_t hash = 0;
    std::string cacheName;
    if ( albedoMap.empty() )
    {
        cacheName += "$white";
    }
    else
    {
        HashCombine( hash, albedoMap );
        cacheName += GetFilenameStem( albedoMap );
    }
    cacheName += "~";

    if ( metalnessMap.empty() )
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


std::string TexturesetCreateInfo::GetNormalRoughImageName() const
{
    size_t hash = 0;
    std::string cacheName;
    if ( normalMap.empty() )
    {
        cacheName += "$default_normals";
    }
    else
    {
        HashCombine( hash, normalMap );
        cacheName += GetFilenameStem( normalMap );
    }
    cacheName += "~";

    if ( roughnessMap.empty() )
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


bool Textureset::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const TexturesetCreateInfo* createInfo = (const TexturesetCreateInfo*)baseInfo;
    name = createInfo->name;

    LOG_ERR( "Textureset::Load not implemented yet (expected to load already converted images through Fastfiles, not to do any compositing or compressing at runtime" );

    return false;
}


bool Textureset::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );

    std::string imageName;
    serializer->Read( imageName );
    albedoMetalTex = AssetManager::Get<GfxImage>( imageName );
    PG_ASSERT( albedoMetalTex, "GfxImage '" + imageName + "' not found for Textureset '" + name + "'s albedoMetalTex" );
    
    serializer->Read( imageName );
    normalRoughTex = AssetManager::Get<GfxImage>( imageName );
    PG_ASSERT( normalRoughTex, "GfxImage '" + imageName + "' not found for Textureset '" + name + "' normalRoughTex" );

    return true;
}


bool Textureset::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
    serializer->Write( name );
    if ( !albedoMetalTex )
    {
        LOG_ERR( "Textureset asset must have a 'albedoMetalTex'. Required texture" );
        return false;
    }
    if ( !normalRoughTex )
    {
        LOG_ERR( "Textureset asset must have a 'normalRoughTex'. Required texture" );
        return false;
    }
    serializer->Write( albedoMetalTex->name );
    serializer->Write( normalRoughTex->name );

    return true;
}

} // namespace PG
