#include "gfx_image_converter.hpp"
#include "asset/types/gfx_image.hpp"
#include "image.hpp"
#include "shared/hash.hpp"


namespace PG
{

std::string GfxImageConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
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
            if ( !info->filenames[i].empty() ) HashCombine( hash, info->filenames[i] );
        }
        cacheName += "_" + std::to_string( hash );
    }
    return cacheName;
}


AssetStatus GfxImageConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    AddFastfileDependency( info->filenames[0] );
    for ( const std::string& filename : info->filenames )
    {
        if ( filename.empty() ) break;

        if ( IsFileOutOfDate( cacheTimestamp, filename ) )
        {
            return AssetStatus::OUT_OF_DATE;
        }
    }
    
    return AssetStatus::UP_TO_DATE;
}

} // namespace PG