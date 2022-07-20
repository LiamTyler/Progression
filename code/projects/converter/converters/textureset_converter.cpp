#include "textureset_converter.hpp"

namespace PG
{

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