#include "ui_layout_converter.hpp"
#include "shared/hash.hpp"

namespace PG
{

void UILayoutConverter::AddReferencedAssetsInternal( ConstDerivedInfoPtr& info )
{
    const std::string absPath = GetAbsPath_UILayoutFilename( info->xmlFilename );
    std::string scriptFName   = GetFilenameMinusExtension( absPath ) + ".lua";
    if ( PathExists( scriptFName ) )
    {
        auto scriptInfo      = std::make_shared<ScriptCreateInfo>();
        scriptInfo->name     = info->name;
        scriptInfo->filename = GetFilenameMinusExtension( info->xmlFilename ) + ".lua"; // createInfos store relative paths
        AddUsedAsset( ASSET_TYPE_SCRIPT, scriptInfo );
    }
}

std::string UILayoutConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    cacheName += "_" + std::to_string( Hash( info->xmlFilename ) );
    const std::string absPath = GetAbsPath_UILayoutFilename( info->xmlFilename );
    std::string scriptFName   = GetFilenameMinusExtension( absPath ) + ".lua";
    if ( PathExists( scriptFName ) )
    {
        cacheName += "_scripted";
    }

    return cacheName;
}

AssetStatus UILayoutConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    const std::string absPath = GetAbsPath_UILayoutFilename( info->xmlFilename );
    AddFastfileDependency( absPath );
    std::string scriptFName = GetFilenameMinusExtension( absPath ) + ".lua";
    if ( PathExists( scriptFName ) )
    {
        AddFastfileDependency( scriptFName );
        if ( IsFileOutOfDate( cacheTimestamp, scriptFName ) )
        {
            return AssetStatus::OUT_OF_DATE;
        }
    }

    return IsFileOutOfDate( cacheTimestamp, absPath ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG
