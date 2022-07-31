#include "textureset_converter.hpp"
#include "core/image_processing.hpp"
#include "shared/hash.hpp"

namespace PG
{


bool TexturesetConverter::Convert( const std::string& assetName )
{
    auto info = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( assetType, assetName );
    LOG( "Converting out of date asset %s %s...", g_assetNames[assetType], info->name.c_str() );

    CompositeImageInput albedoMetalnessInput;
    albedoMetalnessInput.compositeType = CompositeType::REMAP;
    albedoMetalnessInput.outputColorSpace = ColorSpace::SRGB;
    albedoMetalnessInput.sourceImages.resize( 2 );

    albedoMetalnessInput.sourceImages[0].filename = info->albedoMap.empty() ? "$white" : info->albedoMap;
    albedoMetalnessInput.sourceImages[0].colorSpace = ColorSpace::SRGB;
    albedoMetalnessInput.sourceImages[0].remaps.push_back( { Channel::R, Channel::R } );
    albedoMetalnessInput.sourceImages[0].remaps.push_back( { Channel::G, Channel::G } );
    albedoMetalnessInput.sourceImages[0].remaps.push_back( { Channel::B, Channel::B } );

    albedoMetalnessInput.sourceImages[1].filename = info->metalnessMap.empty() ? "$default_metalness" : info->metalnessMap;
    albedoMetalnessInput.sourceImages[1].colorSpace = ColorSpace::LINEAR;
    albedoMetalnessInput.sourceImages[1].remaps.push_back( { Channel::A, Channel::A } );
    

    Textureset textureset;
    std::string cacheName = GetCacheName( info );
    if ( !AssetCache::CacheAsset( assetType, cacheName, &textureset ) )
    {
        LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], assetName.c_str(), cacheName.c_str() );
        return false;
    }

    return true;
}


std::string TexturesetConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash = Hash( info->albedoMap );
    HashCombine( hash, info->metalnessMap );
    HashCombine( hash, info->metalnessScale );
    HashCombine( hash, Underlying( info->metalnessSourceChannel ) );
    HashCombine( hash, info->normalMap );
    HashCombine( hash, info->slopeScale );
    HashCombine( hash, info->roughnessMap );
    HashCombine( hash, Underlying( info->roughnessSourceChannel ) );
    HashCombine( hash, info->invertRoughness );
    cacheName += "_" + std::to_string( hash );
    return cacheName;
}


AssetStatus TexturesetConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    if ( !info->albedoMap.empty() && !IsImageFilenameBuiltin( info->albedoMap ) )
    {
        AddFastfileDependency( info->albedoMap );
        if ( IsFileOutOfDate( cacheTimestamp, info->albedoMap ) ) return AssetStatus::OUT_OF_DATE;
    }
    if ( !info->metalnessMap.empty() && !IsImageFilenameBuiltin( info->metalnessMap ) )
    {
        AddFastfileDependency( info->metalnessMap );
        if ( IsFileOutOfDate( cacheTimestamp, info->metalnessMap ) ) return AssetStatus::OUT_OF_DATE;
    }
    if ( !info->normalMap.empty() && !IsImageFilenameBuiltin( info->normalMap ) )
    {
        AddFastfileDependency( info->normalMap );
        if ( IsFileOutOfDate( cacheTimestamp, info->normalMap ) ) return AssetStatus::OUT_OF_DATE;
    }
    if ( !info->roughnessMap.empty() && !IsImageFilenameBuiltin( info->roughnessMap ) )
    {
        AddFastfileDependency( info->roughnessMap );
        if ( IsFileOutOfDate( cacheTimestamp, info->roughnessMap ) ) return AssetStatus::OUT_OF_DATE;
    }
    //AddFastfileDependency( info->filename );
    //return IsFileOutOfDate( cacheTimestamp, info->filename ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
    return AssetStatus::ERROR;
}

} // namespace PG