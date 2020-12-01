#include "core/assert.hpp"
#include "asset/asset_versions.hpp"
#include "converters/gfx_image_converter.hpp"
#include "converters/material_converter.hpp"
#include "converters/model_converter.hpp"
#include "converters/script_converter.hpp"
#include "converters/shader_converter.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/logger.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"
#include <algorithm>
#include <chrono>

#include "asset/types/shader.hpp"
#include "asset/shader_preprocessor.hpp"
using namespace PG;

bool g_parsingError;
int g_outOfDateAssets;
int g_checkDependencyErrors;

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

    //LOG( "Hello world" );
    //LOG( "Hello world" );


    //DefineList defines =
    //{
    //};
    //defines.emplace_back( "PG_SHADER_CODE", "1" );
    //
    //std::string filename = PG_ASSET_DIR "shaders/model.vert";
    //ShaderPreprocessOutput preproc = PreprocessShader( filename, defines );
    //if ( !preproc.success )
    //{
    //    LOG_ERR( "Error in preproc" );
    //}
    //else
    //{
    //    LOG( "Preproc sucess" );
    //    std::ofstream out( "preproc.glsl" );
    //    out << preproc.outputShader;
    //}

    if ( !RunConverter( argv[1] ) )
    {
        Logger_Shutdown();
        return 0;
    }
    if ( !BuildFastfile( argv[1] ) )
    {
        Logger_Shutdown();
        return 0;
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
    s_latestAssetTimestamp = 0;
    g_checkDependencyErrors = 0;
    g_outOfDateAssets = 0;
    g_parsingError = false;

    LOG( "Running converter..." );
    CreateDirectory( PG_ASSET_DIR "cache/" );
    CreateDirectory( PG_ASSET_DIR "cache/fastfiles/" );
    CreateDirectory( PG_ASSET_DIR "cache/images/" );
    CreateDirectory( PG_ASSET_DIR "cache/materials/" );
    CreateDirectory( PG_ASSET_DIR "cache/models/" );
    CreateDirectory( PG_ASSET_DIR "cache/scripts/" );
    CreateDirectory( PG_ASSET_DIR "cache/shaders/" );

    auto converterStartTime = std::chrono::system_clock::now();
    LOG( "Loading asset list file '%s'...", assetFile.c_str() );
    rapidjson::Document document;
    if ( !ParseJSONFile( assetFile, document ) )
    {
        return false;
    }
    
    static JSONFunctionMapper mapping(
    {
        { "Image",   GfxImage_Parse },
        { "MatFile", Material_Parse },
        { "Script",  Script_Parse },
        { "Model",   Model_Parse },
        { "Shader",  Shader_Parse },
    });

    PG_ASSERT( document.HasMember( "AssetList" ), "asset list file requires a single object 'AssetList' that has a list of all assets" );
    const auto& assetList = document["AssetList"];
    for ( rapidjson::Value::ConstValueIterator itr = assetList.Begin(); itr != assetList.End(); ++itr )
    {
        mapping.ForEachMember( *itr );
    }
    
    if ( g_parsingError )
    {
        LOG_ERR( "Parsing errors, exiting" );
        return false;
    }

    LOG( "Checking asset dependencies..." );
    int numGfxImageOutOfDate = GfxImage_CheckDependencies();
    g_outOfDateAssets += numGfxImageOutOfDate;
    LOG( "GfxImages out of date: %d", numGfxImageOutOfDate );

    int numMatFilesOutOfDate = Material_CheckDependencies();
    g_outOfDateAssets += numMatFilesOutOfDate;
    LOG( "MatFiles out of date: %d", numMatFilesOutOfDate );

    int numScriptFilesOutOfDate = Script_CheckDependencies();
    g_outOfDateAssets += numScriptFilesOutOfDate;
    LOG( "Scripts out of date: %d", numScriptFilesOutOfDate );

    int numModelFilesOutOfDate = Model_CheckDependencies();
    g_outOfDateAssets += numModelFilesOutOfDate;
    LOG( "Models out of date: %d", numModelFilesOutOfDate );

    int numShaderFilesOutOfDate = Shader_CheckDependencies();
    g_outOfDateAssets += numShaderFilesOutOfDate;
    LOG( "Shaders out of date: %d", numShaderFilesOutOfDate );

    if ( g_checkDependencyErrors > 0 )
    {
        LOG_ERR( "%d assets with errors during dependency check phase. FAILED" );
        return false;
    }

    if ( g_outOfDateAssets == 0 )
    {
        LOG( "All assets up to date" );
    }
    else
    {
        LOG( "Converting %d assets...", g_outOfDateAssets );
        int totalErrors = 0;
        totalErrors += GfxImage_Convert();
        totalErrors += Material_Convert();
        totalErrors += Script_Convert();
        totalErrors += Model_Convert();
        totalErrors += Shader_Convert();

        if ( totalErrors )
        {
            LOG_ERR( "%d errors while converting, convert FAILED", totalErrors );
            return false;
        }
        else
        {
            LOG( "Convert SUCCESS" );
        }
    }

    float duration = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - converterStartTime ).count() / 1e6f;
    LOG( "Converter finished in %.2f seconds", duration );

    return true;
}

bool BuildFastfile( const std::string& assetFile )
{
    LOG( "\nRunning build fastfile..." );
    auto startTime = std::chrono::system_clock::now();
    std::string ffName = PG_ASSET_DIR "cache/fastfiles/" + GetFilenameStem( assetFile ) + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    if ( g_outOfDateAssets == 0 && PathExists( ffName ) && GetFileTimestamp( ffName ) > s_latestAssetTimestamp )
    {
        LOG( "Fastfile '%s' is up to date, not rebuilding", ffName.c_str() );
        return true;
    }

    LOG( "Fastfile '%s' is out of date, building...", ffName.c_str() );
    Serializer outFile;
    if ( !outFile.OpenForWrite( ffName ) )
    {
        return false;
    }
    static_assert( NUM_ASSET_TYPES == 5, "Dont forget to update this, otherwise new asset wont be written to fastfile" );
    bool success = true;
    success = success && GfxImage_BuildFastFile( &outFile );
    success = success && Material_BuildFastFile( &outFile );
    success = success && Script_BuildFastFile( &outFile );
    success = success && Model_BuildFastFile( &outFile );
    success = success && Shader_BuildFastFile( &outFile );

    if ( success )
    {
        LOG( "Build fastfile SUCCESS" );
    }
    else
    {
        LOG_ERR( "Build fastfile FAILED" );
        outFile.Close();
        DeleteFile( ffName );
    }

    float duration = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - startTime ).count() / 1e6f;
    LOG( "Build Fastfile finished in %.2f seconds", duration );

    return success;
}