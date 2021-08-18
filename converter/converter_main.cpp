#include "asset/asset_versions.hpp"
#include "core/assert.hpp"
#include "core/time.hpp"
#include "converters/base_asset_converter.hpp"
#include "converters/gfx_image_converter.hpp"
#include "converters/material_converter.hpp"
#include "converters/model_converter.hpp"
#include "converters/script_converter.hpp"
#include "converters/shader_converter.hpp"
#include "utils/cpu_profile.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/logger.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"
#include "getopt/getopt.h"
#include <algorithm>

using namespace PG;

static void DisplayHelp()
{
    auto msg =
      "Usage: converter [options] path_to_assetlist.json\n"
      "Options\n"
      "  --force                     Don't check asset file dependencies, just reconvert everything\n"
      "  --help                      Print this message and exit\n"
      "  --generateDebugInfo         Generate debug info when compiling spirv, and add #define IS_DEBUG_SHADER 1 to shader\n"
      "  --saveShaderPreproc         Save the shader preproc (to assets/cache/shader_preproc/)\n";

    LOG( "%s", msg );   
}

bool ConvertAssets( const std::string& assetFile, const std::vector< BaseAssetConverter* >& converters );

bool AssembleConvertedAssetsIntoFastfile( const std::string& assetFile, const std::vector< BaseAssetConverter* >& converters );

int main( int argc, char** argv )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    if ( argc == 1 )
    {
        DisplayHelp();
        return 0;
    }

    static struct option long_options[] =
    {
        { "force",                no_argument,  0, 'f' },
        { "help",                 no_argument,  0, 'h' },
        { "generateDebugInfo",    no_argument,  0, 'd' },
        { "saveShaderPreproc",    no_argument,  0, 's' },
        { 0, 0, 0, 0 }
    };

    g_converterConfigOptions = {};
    int option_index = 0;
    int c            = -1;
    while ( ( c = getopt_long( argc, argv, "dfhs", long_options, &option_index ) ) != -1 )
    {
        switch ( c )
        {
            case 'd':
                g_converterConfigOptions.generateShaderDebugInfo = true;
                break;
            case 'f':
                g_converterConfigOptions.force = true;
                break;
            case 'h':
                DisplayHelp();
                return 0;
            case 's':
                g_converterConfigOptions.saveShaderPreproc = true;
                break;
            case '?':
            default:
                LOG_ERR( "Invalid option, try 'converter --help' for more information" );
                return 0;
        }
    }

    if ( optind >= argc )
    {
        DisplayHelp();
        return 0;
    }
    std::string assetListFile = argv[optind];
    std::vector< BaseAssetConverter* > converters =
    {
        new GfxImageConverter,
        new ShaderConverter,
        new ScriptConverter,
        new MaterialFileConverter,
        new ModelConverter,
    };

    if ( ConvertAssets( assetListFile, converters ) )
    {
        AssembleConvertedAssetsIntoFastfile( assetListFile, converters );
    }

    Logger_Shutdown();
    return 0;
}


static int s_outOfDateAssets;


bool ConvertAssets( const std::string& assetListFile, const std::vector< BaseAssetConverter* >& converters )
{
    ClearAllFastfileDependencies();
    s_outOfDateAssets       = 0;
    g_converterStatus       = {};

    LOG( "Running converter..." );
    CreateDirectory( PG_ASSET_DIR "cache/" );
    CreateDirectory( PG_ASSET_DIR "cache/fastfiles/" );
    CreateDirectory( PG_ASSET_DIR "cache/images/" );
    CreateDirectory( PG_ASSET_DIR "cache/materials/" );
    CreateDirectory( PG_ASSET_DIR "cache/models/" );
    CreateDirectory( PG_ASSET_DIR "cache/scripts/" );
    CreateDirectory( PG_ASSET_DIR "cache/shaders/" );
    CreateDirectory( PG_ASSET_DIR "cache/shader_preproc/" );

    auto convertAssetsStartTime = PG::Time::GetTimePoint();
    LOG( "Loading asset list file '%s'...", assetListFile.c_str() );
    rapidjson::Document document;
    if ( !ParseJSONFile( assetListFile, document ) )
    {
        return false;
    }
    if ( !document.HasMember( "AssetList" ) )
    {
        LOG_ERR( "Asset list file requires a single object 'AssetList' that has a list of all assets" );
        return false;
    }
    
    JSONFunctionMapper<>::StringToFuncMap map;
    for ( const auto& converter : converters )
    {
        map.emplace( converter->AssetNameInJSONFile(), [&]( const rapidjson::Value& value ) { converter->Parse( value ); } );
    }
    JSONFunctionMapper mapping( map );

    const auto& assetList = document["AssetList"];
    for ( rapidjson::Value::ConstValueIterator itr = assetList.Begin(); itr != assetList.End(); ++itr )
    {
        mapping.ForEachMember( *itr );
    }
    
    if ( g_converterStatus.parsingError )
    {
        LOG_ERR( "Parsing errors, exiting" );
        return false;
    }

    LOG( "Checking asset dependencies..." );
    for ( const auto& converter : converters )
    {
        int outOfDate = converter->CheckAllDependencies();
        s_outOfDateAssets += outOfDate;
        LOG( "%s out of date: %d", converter->AssetNameInJSONFile().c_str(), outOfDate );
    }

    bool status = true;
    if ( g_converterStatus.checkDependencyErrors > 0 )
    {
        LOG_ERR( "%d assets with errors during dependency check phase. FAILED" );
        status = false;
    }
    else
    {
        if ( s_outOfDateAssets == 0 )
        {
            LOG( "All assets up to date" );
        }
        else
        {
            LOG( "Converting %d assets...", s_outOfDateAssets );
            int totalConvertErrors = 0;
            for ( const auto& converter : converters )
            {
                totalConvertErrors += converter->ConvertAll();
            }

            if ( totalConvertErrors )
            {
                LOG_ERR( "%d errors while converting, convert FAILED", totalConvertErrors );
                status = false;
            }
            else
            {
                LOG( "Convert SUCCESS" );
            }
        }
    }

    double elapsedTime = PG::Time::GetDuration( convertAssetsStartTime ) / 1000.0;
    if ( status )
    {
        LOG( "Elapsed time: %.2f seconds", elapsedTime );
    }
    else
    {
        LOG_ERR( "Elapsed time: %.2f seconds", elapsedTime );
    }

    return status;
}


bool AssembleConvertedAssetsIntoFastfile( const std::string& assetListFile, const std::vector< BaseAssetConverter* >& converters )
{
    auto buildFastfileStartTime = PG::Time::GetTimePoint();
    LOG( "\nRunning build fastfile..." );
    auto startTime = std::chrono::system_clock::now();
    std::string ffName = PG_ASSET_DIR "cache/fastfiles/" + GetFilenameStem( assetListFile ) + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    if ( s_outOfDateAssets == 0 && PathExists( ffName ) && GetFileTimestamp( ffName ) >= GetLatestFastfileDependency() )
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
    bool success = true;
    for ( const auto& converter : converters )
    {
        success = success && converter->BuildFastFile( &outFile );
    }

    double elapsedTime = PG::Time::GetDuration( buildFastfileStartTime ) / 1000.0;
    if ( success )
    {
        LOG( "Build fastfile SUCCESS" );
        LOG( "Elapsed time: %.2f seconds", elapsedTime );
    }
    else
    {
        LOG_ERR( "Build fastfile FAILED" );
        LOG_ERR( "Elapsed time: %.2f seconds", elapsedTime );
        outFile.Close();
        DeleteFile( ffName );
    }

    return success;
}