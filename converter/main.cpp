#include "converter.hpp"
#include "core/assert.hpp"
#include "asset/asset_versions.hpp"
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

//#include "assetTypes/model.hpp"
//#include "asset_manager.hpp"
//#include "assetTypes/shader.hpp"
//using namespace PG;


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

bool ConvertAssets();

bool AssembleConvertedAssetsIntoFastfile();

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

    g_converterConfigOptions.assetListFile = argv[optind];

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

    g_converterStatus.convertAssetErrors = !ConvertAssets();
    g_converterStatus.buildFastfileErrors = AssembleConvertedAssetsIntoFastfile();

    //Shader shader;
    //ShaderCreateInfo info;
    //info.name = "gbufferFrag";
    //info.shaderStage = ShaderStage::FRAGMENT;
    //info.filename = PG_ASSET_DIR "shaders/gbuffer.frag";
    //if ( !Shader_Load( &shader, info ) )
    //{
    //    return 0;
    //}
    //AssetManager::Init();
    //AssetManager::LoadFastFile( "assetList" );
    //Shader* asset = AssetManager::Get< Shader >( "gbufferFrag" );
    //PG_ASSERT( asset->name == shader.name );
    //PG_ASSERT( asset->shaderStage == shader.shaderStage );
    //PG_ASSERT( asset->name == shader.name );
    //PG_ASSERT( asset->spirv == shader.spirv );

    Logger_Shutdown();
    return 0;
}


static time_t s_latestAssetTimestamp;
static int s_outOfDateAssets;


void AddFastfileDependency( const std::string& file )
{
    s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, GetFileTimestamp( file ) );
}


bool ConvertAssets()
{
    s_outOfDateAssets       = 0;
    s_latestAssetTimestamp  = 0;
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

    PG_PROFILE_CPU_START( CONVERTER );
    LOG( "Loading asset list file '%s'...", g_converterConfigOptions.assetListFile.c_str() );
    rapidjson::Document document;
    if ( !ParseJSONFile( g_converterConfigOptions.assetListFile, document ) )
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


bool AssembleConvertedAssetsIntoFastfile()
{
    if ( g_converterStatus.convertAssetErrors )
    {
        return false;
    }

    PG_PROFILE_CPU_START( BUILDFASTFILE );
    LOG( "\nRunning build fastfile..." );
    auto startTime = std::chrono::system_clock::now();
    std::string ffName = PG_ASSET_DIR "cache/fastfiles/" + GetFilenameStem( g_converterConfigOptions.assetListFile ) + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
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