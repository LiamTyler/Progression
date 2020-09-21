#include "core/assert.hpp"
#include "asset/types/model.hpp"
#include "asset/asset_versions.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

using namespace PG;

extern void AddFastfileDependency( const std::string& file );

std::vector< ModelCreateInfo > g_parsedModels;
std::vector< ModelCreateInfo > g_outOfDateModels;
extern bool g_parsingError;

void Model_Parse( const rapidjson::Value& value )
{
    static FunctionMapper< ModelCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.name = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, ModelCreateInfo& i ) { i.filename = PG_ASSET_DIR "models/" + std::string( v.GetString() ); } },
    });

    ModelCreateInfo info;
    mapping.ForEachMember( value, info );

    if ( !FileExists( info.filename ) )
    {
        LOG_ERR( "Filename '%s' not found for Model '%s', skipping model\n", info.filename.c_str(), info.name.c_str() );
        g_parsingError = true;
    }

    g_parsedModels.push_back( info );
}


static std::string Model_GetFastFileName( const ModelCreateInfo& info )
{
    static_assert( sizeof( ModelCreateInfo ) == 2 * sizeof( std::string ), "Dont forget to add new hash value" );

    std::string baseName = info.name;
    baseName += "_v" + std::to_string( PG_GFX_IMAGE_VERSION );
    baseName += "_" + std::to_string( std::hash< std::string >{}( info.filename ) );

    std::string fullName = PG_ASSET_DIR "cache/models/" + baseName + ".ffi";
    return fullName;
}


static bool Model_IsOutOfDate( const ModelCreateInfo& info )
{
    std::string ffName = Model_GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info.filename );
}


static bool Model_ConvertSingle( const ModelCreateInfo& info )
{
    LOG( "Converting model '%s'...\n", info.name.c_str() );
    Model model;
    std::vector< std::string > materialNames;
    if ( !Model_Load_NoResolveMaterials( &model, info, materialNames ) )
    {
        return false;
    }
    std::string fastfileName = Model_GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_Model_Save( &model, &serializer, materialNames ) )
    {
        LOG_ERR( "Error while writing model '%s' to fastfile\n", model.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    serializer.Close();

    return true;
}


int Model_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < g_parsedModels.size(); ++i )
    {
        if ( Model_IsOutOfDate( g_parsedModels[i] ) )
        {
            g_outOfDateModels.push_back( g_parsedModels[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


int Model_Convert()
{
    if ( g_outOfDateModels.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)g_outOfDateModels.size(); ++i )
    {
        if ( !Model_ConvertSingle( g_outOfDateModels[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


bool Model_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < g_parsedModels.size(); ++i )
    {
        std::string ffiName = Model_GetFastFileName( g_parsedModels[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'\n", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_MODEL );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}