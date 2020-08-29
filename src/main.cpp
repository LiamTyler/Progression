#include "assert.hpp"
#include "asset_versions.hpp"
#include "converters/gfx_image_converter.hpp"
#include "converters/material_converter.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/logger.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"
#include <chrono>

#include "asset_manager.hpp"
#include "gfx_image.hpp"
#include "material.hpp"

bool g_parsingError;
int g_outOfDateAssets;

static time_t s_latestAssetTimestamp;

void AddFastfileDependency( const std::string& file );

bool RunConverter( const std::string& assetFile );

bool BuildFastfile( const std::string& assetFile );

int main( int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf( "Usage: converter [path_to_assetlist.json]\n" );
        return 0;
    }

    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    
    if ( RunConverter( argv[1] ) )
    {
        if ( !BuildFastfile( argv[1] ) )
        {
            LOG_ERR( "Build fastfile failed\n" );
        }
        else
        {
            LOG( "Fastfile build SUCCESS\n" );
        }
    }

    if ( !Progression::AssetManager::LoadFastFile( "assetList" ) )
    {
        LOG_ERR( "Could not laod fastfile!\n" );
    }
    else
    {
        Progression::GfxImage* img = Progression::AssetManager::Get< Progression::GfxImage >( "macaw" );
        PG_ASSERT( img );
        LOG( "name = %s\n", img->name.c_str() );
        LOG( "width = %d\n", img->width );
        LOG( "height = %d\n", img->height );
        LOG( "depth = %d\n", img->depth );

        Progression::Material* mat = Progression::AssetManager::Get< Progression::Material >( "blue" );
        PG_ASSERT( mat );
        LOG( "name = %s\n", mat->name.c_str() );
        LOG( "Kd = (%d, %d, %d)\n", mat->Kd.x, mat->Kd.y, mat->Kd.z );
        LOG( "mapKd = %#08x\n", mat->map_Kd );
    }

    Logger_Shutdown();
    return 0;
}


void AddFastfileDependency( const std::string& file )
{
    s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, GetFileTimestamp( file ) );
}


bool RunConverter( const std::string& assetFile )
{
    LOG( "Running converter...\n" );
    s_latestAssetTimestamp = 0;
    CreateDirectory( PG_ASSET_DIR "cache/" );
    CreateDirectory( PG_ASSET_DIR "cache/fastfiles/" );
    CreateDirectory( PG_ASSET_DIR "cache/images/" );
    CreateDirectory( PG_ASSET_DIR "cache/materials/" );

    auto converterStartTime = std::chrono::system_clock::now();
    LOG( "Loading asset list file '%s'...\n", assetFile.c_str() );
    rapidjson::Document document;
    if ( !ParseJSONFile( assetFile, document ) )
    {
        return false;
    }
    
    static FunctionMapper mapping(
    {
        { "Image",   GfxImage_Parse },
        { "MatFile", Material_Parse },
    });

    g_parsingError = false;

    PG_ASSERT( document.HasMember( "AssetList" ), "asset list file requires a single object 'AssetList' that has a list of all assets" );
    const auto& assetList = document["AssetList"];
    for ( rapidjson::Value::ConstValueIterator itr = assetList.Begin(); itr != assetList.End(); ++itr )
    {
        mapping.ForEachMember( *itr );
    }
    
    if ( g_parsingError )
    {
        LOG_ERR( "Parsing errors, exiting\n" );
        return false;
    }

    LOG( "Checking asset dependencies...\n" );
    int numGfxImageOutOfDate = GfxImage_CheckDependencies();
    LOG( "GfxImages out of date: %d\n", numGfxImageOutOfDate );
    int numMatFilesOutOfDate = Material_CheckDependencies();
    LOG( "MatFiles out of date: %d\n", numMatFilesOutOfDate );

    g_outOfDateAssets = numGfxImageOutOfDate + numMatFilesOutOfDate;
    if ( g_outOfDateAssets == 0 )
    {
        LOG( "All assets up to date\n" );
    }
    else
    {
        LOG( "Converting %d assets...\n", g_outOfDateAssets );
        int totalErrors = 0;
        totalErrors += GfxImage_Convert();
        totalErrors += Material_Convert();

        if ( totalErrors )
        {
            LOG_ERR( "%d errors while converting, convert failed\n", totalErrors );
            return false;
        }
        else
        {
            LOG( "SUCCESS\n" );
        }
    }

    float duration = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - converterStartTime ).count() / 1e6f;
    LOG( "Converter finished in %.2f seconds\n", duration );

    return true;
}

bool BuildFastfile( const std::string& assetFile )
{
    LOG( "\nRunning build fastfile...\n" );
    auto startTime = std::chrono::system_clock::now();
    std::string ffName = PG_ASSET_DIR "cache/fastfiles/" + GetFilenameStem( assetFile ) + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    if ( g_outOfDateAssets == 0 && FileExists( ffName ) && GetFileTimestamp( ffName ) > s_latestAssetTimestamp )
    {
        LOG( "Fastfile '%s' is up to date, not rebuilding\n", ffName.c_str() );
        return true;
    }

    LOG( "Fastfile '%s' is out of date, building...\n", ffName.c_str() );
    Serializer outFile;
    if ( !outFile.OpenForWrite( ffName ) )
    {
        return false;
    }
    static_assert( NUM_ASSET_TYPES == 2, "Dont forget to update this, otherwise new asset wont be written to fastfile" );
    bool success = true;
    success = success && GfxImage_BuildFastFile( &outFile );
    success = success && Material_BuildFastFile( &outFile );

    float duration = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - startTime ).count() / 1e6f;
    LOG( "Build Fastfile finished in %.2f seconds\n", duration );

    return success;
}