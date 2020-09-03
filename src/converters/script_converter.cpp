#include "assert.hpp"
#include "assetTypes/script.hpp"
#include "asset_versions.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

using namespace Progression;

extern void AddFastfileDependency( const std::string& file );

std::vector< ScriptCreateInfo > g_parsedScripts;
std::vector< ScriptCreateInfo > g_outOfDateScripts;
extern bool g_parsingError;


void Script_Parse( const rapidjson::Value& value )
{

    static FunctionMapper< ScriptCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.name     = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, ScriptCreateInfo& s ) { s.filename = PG_ASSET_DIR "scripts/" + std::string( v.GetString() ); } },
    });

    ScriptCreateInfo info;
    mapping.ForEachMember( value, info );

    if ( !FileExists( info.filename ) )
    {
        LOG_ERR( "Script file '%s' not found\n", info.filename.c_str() );
        g_parsingError = true;
    }

    g_parsedScripts.push_back( info );
}


static std::string Script_GetFastFileName( const ScriptCreateInfo& info )
{
    std::string baseName = info.name;
    baseName += "_" + GetFilenameStem( info.filename );
    baseName += "_v" + std::to_string( PG_SCRIPT_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/scripts/" + baseName + ".ffi";
    return fullName;
}


static bool Script_IsOutOfDate( const ScriptCreateInfo& info )
{
    std::string ffName = Script_GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info.filename );
}


static bool Script_ConvertSingle( const ScriptCreateInfo& info )
{
    LOG( "Converting Script file '%s'...\n", info.filename.c_str() );
    Script asset;
    if ( !Script_Load( &asset, info ) )
    {
        return false;
    }
    std::string fastfileName = Script_GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_Script_Save( &asset, &serializer ) )
    {
        LOG_ERR( "Error while writing script '%s' to fastfile\n", info.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    
    serializer.Close();

    return true;
}


int Script_Convert()
{
    if ( g_outOfDateScripts.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)g_outOfDateScripts.size(); ++i )
    {
        if ( !Script_ConvertSingle( g_outOfDateScripts[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


int Script_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < g_parsedScripts.size(); ++i )
    {
        if ( Script_IsOutOfDate( g_parsedScripts[i] ) )
        {
            g_outOfDateScripts.push_back( g_parsedScripts[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


bool Script_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < g_parsedScripts.size(); ++i )
    {
        std::string ffiName = Script_GetFastFileName( g_parsedScripts[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'\n", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_SCRIPT );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}