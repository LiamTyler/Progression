#include "model_converter.hpp"

namespace PG
{

std::string ModelConverter::GetCacheNameInternal( ConstInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + std::to_string( std::hash<std::string>{}( info->filename ) );
    return cacheName;
}


AssetStatus ModelConverter::IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp )
{
    AddFastfileDependency( info->filename );
    return IsFileOutOfDate( cacheTimestamp, info->filename ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG