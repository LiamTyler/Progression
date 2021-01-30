#include "converter.hpp"
#include "asset/asset_manager.hpp"
#include "asset/asset_versions.hpp"
#include "converters/gfx_image_converter.hpp"
#include "converters/material_converter.hpp"
#include "converters/model_converter.hpp"
#include "converters/script_converter.hpp"
#include "converters/shader_converter.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "generate_missing_pipelines.hpp"
#include "utils/cpu_profile.hpp"
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

ConverterConfigOptions g_converterConfigOptions;
ConverterStatus g_converterStatus;

void AddFastfileDependency( const std::string& file );

bool ConvertAssets( const std::string& assetFile );

bool AssembleConvertedAssetsIntoFastfile( const std::string& assetFile );

#define EXIT_CONVERTER do { Logger_Shutdown(); return 0; } while ( 0 )

int main( int argc, char** argv )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    if ( argc == 1 )
    {
        DisplayHelp();
        EXIT_CONVERTER;
    }

    CreateDirectory( PG_ASSET_DIR "cache/" );
    CreateDirectory( PG_ASSET_DIR "cache/fastfiles/" );
    CreateDirectory( PG_ASSET_DIR "cache/images/" );
    CreateDirectory( PG_ASSET_DIR "cache/materials/" );
    CreateDirectory( PG_ASSET_DIR "cache/models/" );
    CreateDirectory( PG_ASSET_DIR "cache/scripts/" );
    CreateDirectory( PG_ASSET_DIR "cache/shaders/" );
    CreateDirectory( PG_ASSET_DIR "cache/shader_preproc/" );

    static struct option long_options[] =
    {
        { "force",                no_argument,  0, 'f' },
        { "help",                 no_argument,  0, 'h' },
        { "generateDebugInfo",    no_argument,  0, 'd' },
        { "saveShaderPreproc",    no_argument,  0, 's' },
        { 0, 0, 0, 0 }
    };

    g_converterConfigOptions = {};
    g_converterStatus = {};
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
                EXIT_CONVERTER;
        }
    }

    if ( optind >= argc )
    {
        DisplayHelp();
        EXIT_CONVERTER;
    }

    AssetManager::Init();

    g_converterConfigOptions.assetListFile = argv[optind];
    //Scene* scene = Scene::Load( g_converterConfigOptions.assetListFile );
    Scene* scene = Scene::Load( PG_ASSET_DIR "scenes/singleMeshViewer.json" );
    if ( !scene )
    {
        EXIT_CONVERTER;
    }
    LOG( "Scene loaded successfully" );
    bool success = true;
    for ( size_t i = 0; success && i < g_converterStatus.assetFiles.size(); ++i )
    {
        std::string assetFile = PG_ASSET_DIR "fastfiles/" + g_converterStatus.assetFiles[i] + ".json";
        success = success && ConvertAssets( assetFile );
        success = success && AssembleConvertedAssetsIntoFastfile( assetFile );
    }
    if ( !success )
    {
        EXIT_CONVERTER;
    }

    GenerateMissingPipelines( scene );

    EXIT_CONVERTER;
}


static time_t s_latestAssetTimestamp;
static int s_outOfDateAssets;


void AddFastfileDependency( const std::string& file )
{
    s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, GetFileTimestamp( file ) );
}


bool ConvertAssets( const std::string& assetFile )
{
    s_outOfDateAssets       = 0;
    s_latestAssetTimestamp  = 0;

    LOG( "Running ConvertAssets on file '%s'...", assetFile.c_str() );

    PG_PROFILE_CPU_START( CONVERTER );
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
    
    if ( g_converterStatus.parsingError )
    {
        LOG_ERR( "Parsing errors, exiting" );
        return false;
    }

    LOG( "Checking asset dependencies..." );
    int numGfxImageOutOfDate = GfxImage_CheckDependencies();
    s_outOfDateAssets += numGfxImageOutOfDate;
    LOG( "GfxImages out of date: %d", numGfxImageOutOfDate );

    int numMatFilesOutOfDate = Material_CheckDependencies();
    s_outOfDateAssets += numMatFilesOutOfDate;
    LOG( "MatFiles out of date: %d", numMatFilesOutOfDate );

    int numScriptFilesOutOfDate = Script_CheckDependencies();
    s_outOfDateAssets += numScriptFilesOutOfDate;
    LOG( "Scripts out of date: %d", numScriptFilesOutOfDate );

    int numModelFilesOutOfDate = Model_CheckDependencies();
    s_outOfDateAssets += numModelFilesOutOfDate;
    LOG( "Models out of date: %d", numModelFilesOutOfDate );

    int numShaderFilesOutOfDate = Shader_CheckDependencies();
    s_outOfDateAssets += numShaderFilesOutOfDate;
    LOG( "Shaders out of date: %d", numShaderFilesOutOfDate );

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
            int totalErrors = 0;
            totalErrors += GfxImage_Convert();
            totalErrors += Material_Convert();
            totalErrors += Script_Convert();
            totalErrors += Model_Convert();
            totalErrors += Shader_Convert();

            if ( totalErrors )
            {
                LOG_ERR( "%d errors while converting, convert FAILED", totalErrors );
                status = false;
            }
            else
            {
                LOG( "Convert SUCCESS" );
            }
        }
    }

    float elapsedTime = PG_PROFILE_CPU_GET_DURATION( CONVERTER ) / 1000.0f;
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


bool AssembleConvertedAssetsIntoFastfile( const std::string& assetFile )
{
    PG_PROFILE_CPU_START( BUILDFASTFILE );
    LOG( "\nRunning build fastfile..." );
    auto startTime = std::chrono::system_clock::now();
    std::string ffName = PG_ASSET_DIR "cache/fastfiles/" + GetFilenameStem( assetFile ) + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    if ( s_outOfDateAssets == 0 && PathExists( ffName ) && GetFileTimestamp( ffName ) > s_latestAssetTimestamp )
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

    float elapsedTime = PG_PROFILE_CPU_GET_DURATION( BUILDFASTFILE ) / 1000.0f;
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