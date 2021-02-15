#include "script_converter.hpp"
#include "asset/types/script.hpp"

namespace PG
{

void ScriptConverter::Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< ScriptCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.name     = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    ScriptCreateInfo* info = new ScriptCreateInfo;
    mapping.ForEachMember( value, *info );

    if ( !PathExists( info->filename ) )
    {
        LOG_ERR( "Script file '%s' not found", info->filename.c_str() );
        g_converterStatus.parsingError = true;
    }

    m_parsedAssets.push_back( info );
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