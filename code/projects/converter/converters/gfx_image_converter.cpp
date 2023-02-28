#include "gfx_image_converter.hpp"
#include "asset/types/gfx_image.hpp"
#include "image.hpp"
#include "shared/hash.hpp"


namespace PG
{

std::string GfxImageConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    uint32_t numImages = 0;
    std::string cacheName = info->name;
    // composite image names are already a valid cache name. No need to append a 2nd hash
    if ( !IsSemanticComposite( info->semantic ) )
    {
        size_t hash = 0;
        HashCombine( hash, Underlying( info->semantic ) );
        HashCombine( hash, Underlying( info->dstPixelFormat ) );
        HashCombine( hash, Underlying( info->flipVertically ) );
        for ( int i = 0; i < 6; ++i )
        {
            if ( info->filenames[i].empty() )
                continue;
            HashCombine( hash, info->filenames[i] );
            ++numImages;
        }
        cacheName += "_" + std::to_string( hash );
    }
    else
    {
        for ( int i = 0; i < 6; ++i )
        {
            if ( !info->filenames[i].empty() )
                 ++numImages;
        }
    }
    PG_ASSERT( numImages > 0, "GfxImage assets must have at least 1 filename specified" );
    return cacheName;
}


AssetStatus GfxImageConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    for ( int i = 0; i < 6; ++i )
    {
        const std::string& filename = info->filenames[i];
        if ( filename.empty() )
            break;

        if ( IsImageFilenameBuiltin( filename ) )
            continue;

        const std::string absPath = PG_ASSET_DIR + filename;
        AddFastfileDependency( absPath );
        if ( IsFileOutOfDate( cacheTimestamp, absPath ) )
            return AssetStatus::OUT_OF_DATE;
    }
    
    return AssetStatus::UP_TO_DATE;
}

} // namespace PG