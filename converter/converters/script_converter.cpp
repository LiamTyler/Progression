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
    ScriptCreateInfo* info = (ScriptCreateInfo*)baseInfo;
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

    ScriptCreateInfo* info = (ScriptCreateInfo*)baseInfo;
    std::string ffName = GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info->filename );
}


bool ScriptConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    ScriptCreateInfo* info = (ScriptCreateInfo*)baseInfo;
    LOG( "Converting Script file '%s'...", info->filename.c_str() );
    Script asset;
    if ( !Script_Load( &asset, *info ) )
    {
        return false;
    }
    std::string fastfileName = GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_Script_Save( &asset, &serializer ) )
    {
        LOG_ERR( "Error while writing script '%s' to fastfile", info->name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    
    serializer.Close();

    return true;
}

} // namespace PG