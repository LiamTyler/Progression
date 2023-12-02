#include "script_converter.hpp"
#include "shared/hash.hpp"

namespace PG
{

std::string ScriptConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + std::to_string( Hash( info->filename ) );
    return cacheName;
}

AssetStatus ScriptConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    const std::string absFilename = GetAbsPath_ScriptFilename( info->filename );
    AddFastfileDependency( absFilename );
    return IsFileOutOfDate( cacheTimestamp, absFilename ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG
