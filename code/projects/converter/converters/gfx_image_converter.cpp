#include "gfx_image_converter.hpp"
#include "asset/types/gfx_image.hpp"
#include "image.hpp"
#include "shared/hash.hpp"


namespace PG
{

std::string GfxImageConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + std::to_string( static_cast< int >( info->semantic ) );
    cacheName += "_" + std::to_string( static_cast< int >( info->imageType ) );
    cacheName += "_" + std::to_string( static_cast< int >( info->dstPixelFormat ) );
    cacheName += "_" + std::to_string( static_cast< int >( info->flipVertically ) );
    size_t inputFileHash = 0;
    if ( info->inputType == ImageInputType::INDIVIDUAL_FACES )
    {
        for ( int i = 0; i < 6; ++i )
        {
            HashCombine( inputFileHash, info->faceFilenames[i] );
        }
    }
    else
    {
        inputFileHash = Hash( info->filename );
    }
    cacheName += "_" + std::to_string( inputFileHash );
    return cacheName;
}


AssetStatus GfxImageConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    AddFastfileDependency( info->filename );
    for ( const auto& faceFilename : info->faceFilenames )
    {
        if ( IsFileOutOfDate( cacheTimestamp, faceFilename ) )
        {
            return AssetStatus::OUT_OF_DATE;
        }
    }
    
    return IsFileOutOfDate( cacheTimestamp, info->filename ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG