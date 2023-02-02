#include "ui_layout_converter.hpp"

namespace PG
{

std::string UILayoutConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + GetFilenameStem( info->xmlFilename );
    return cacheName;
}


AssetStatus UILayoutConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    AddFastfileDependency( info->xmlFilename );
    return IsFileOutOfDate( cacheTimestamp, info->xmlFilename ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG