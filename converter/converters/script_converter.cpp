#include "script_converter.hpp"
#include "asset/types/script.hpp"

namespace PG
{

std::shared_ptr<BaseAssetCreateInfo> ScriptConverter::Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent )
{
    static JSONFunctionMapper< ScriptCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.name     = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    auto info = std::make_shared<ScriptCreateInfo>();
    if ( parent )
    {
        *info = *std::static_pointer_cast<const ScriptCreateInfo>( parent );
    }
    mapping.ForEachMember( value, *info );

    return info;
}


std::string ScriptConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    const ScriptCreateInfo* info = (const ScriptCreateInfo*)baseInfo;
    std::string baseName = info->name;
    baseName += "_" + GetFilenameStem( info->filename );
    baseName += "_v" + std::to_string( PG_SCRIPT_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/scripts/" + baseName + ".ffi";
    return fullName;
}


bool ScriptConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    const ScriptCreateInfo* info = (const ScriptCreateInfo*)baseInfo;
    std::string ffName = GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info->filename );
}


bool ScriptConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    return ConvertSingleInternal< Script, ScriptCreateInfo >( baseInfo );
}

} // namespace PG