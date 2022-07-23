#include "textureset_converter.hpp"
#include "core/image_processing.hpp"

namespace PG
{




bool TexturesetConverter::Convert( const std::string& assetName )
{
    auto info = AssetDatabase::FindAssetInfo<TexturesetCreateInfo>( assetType, assetName );
    LOG( "Converting out of date asset %s %s...", g_assetNames[assetType], info->name.c_str() );

    CompositeImageInput albedoMetalnessInput;
    albedoMetalnessInput.compositeType = CompositeType::REMAP;
    albedoMetalnessInput.saveAsSRGBFormat = true;
    albedoMetalnessInput.sourceImages[0] = info->albedoMap.empty() ? "$white" : info->albedoMap;
    albedoMetalnessInput.sourceImages[1] = info->metalnessMap.empty() ? "$default_metalness" : info->metalnessMap;
    albedoMetalnessInput.outputChannelInfos.push_back( { 0, 0, 0, 1, 0 } ); // output R = albedo R
    albedoMetalnessInput.outputChannelInfos.push_back( { 1, 0, 1, 1, 0 } ); // output G = albedo G
    albedoMetalnessInput.outputChannelInfos.push_back( { 2, 0, 2, 1, 0 } ); // output B = albedo B
    albedoMetalnessInput.outputChannelInfos.push_back( { 3, 1, 0, 1, 0 } ); // output A = metalness R


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
    //cacheName += "_" + GetFilenameStem( info->filename );
    return cacheName;
}


AssetStatus TexturesetConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    //AddFastfileDependency( info->filename );
    //return IsFileOutOfDate( cacheTimestamp, info->filename ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
    return AssetStatus::ERROR;
}

} // namespace PG