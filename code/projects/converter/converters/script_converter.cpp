#include "script_converter.hpp"

namespace PG
{

std::string ScriptConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + GetFilenameStem( info->filename );
    return cacheName;
}


AssetStatus ScriptConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    AddFastfileDependency( info->filename );
    return IsFileOutOfDate( cacheTimestamp, info->filename ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG