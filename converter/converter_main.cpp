#include "asset/asset_versions.hpp"
#include "asset/asset_manager.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "utils/cpu_profile.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/logger.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"
#include "getopt/getopt.h"
#include "converters/base_asset_converter.hpp"
#include "ecs/components/model_renderer.hpp"
#include "asset_file_database.hpp"
#include <algorithm>

using namespace PG;

static void DisplayHelp()
{
    auto msg =
      "Usage: converter [options] SCENE_FILE\n"
      "Options\n"
      "  --force                     Don't check asset file dependencies, just reconvert everything\n"
      "  --help                      Print this message and exit\n";

    LOG( "%s", msg );
}

static bool ParseCommandLineArgs( int argc, char** argv, std::string& sceneFile )
{
    if ( argc == 1 )
    {
        DisplayHelp();
        return 0;
    }

    static struct option long_options[] =
    {
        { "force",                no_argument,  0, 'f' },
        { "help",                 no_argument,  0, 'h' },
        { 0, 0, 0, 0 }
    };

    int option_index = 0;
    int c            = -1;
    while ( ( c = getopt_long( argc, argv, "fh", long_options, &option_index ) ) != -1 )
    {
        switch ( c )
        {
            case 'f':
                g_converterConfigOptions.force = true;
                break;
            case 'h':
                DisplayHelp();
                return false;
            default:
                LOG_ERR( "Invalid option, try 'converter --help' for more information" );
                return false;
        }
    }

    if ( optind >= argc )
    {
        DisplayHelp();
        return false;
    }
    sceneFile = PG_ASSET_DIR + std::string( argv[optind] );
    return true;
}


static int s_outOfDateAssets;
bool ConvertAssets( const std::string& sceneFile )
{
    ClearAllFastfileDependencies();
    s_outOfDateAssets       = 0;
    g_converterStatus       = {};
    auto convertAssetsStartTime = Time::GetTimePoint();

    return true;
}


void FindAssetsUsedInScene( const std::string& sceneFile, std::vector<std::string> assetsUsed[AssetType::NUM_ASSET_TYPES] )
{
    Scene* scene = Scene::Load( sceneFile );

    for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::NUM_ASSET_TYPES; ++assetTypeIdx )
    {
        assetsUsed[assetTypeIdx].reserve( AssetManager::g_resourceMaps[assetTypeIdx].size() );
        for ( auto [assetName, _] : AssetManager::g_resourceMaps[assetTypeIdx] )
        {
            assetsUsed[assetTypeIdx].push_back( assetName );
        }
    }

    scene->registry.view<ModelRenderer>().each( [&]( ModelRenderer& modelRenderer )
    {
        if ( modelRenderer.materials.empty() )
        {
            auto baseInfo = AssetDatabase::FindAssetInfo( ASSET_TYPE_MODEL, modelRenderer.model->name );
            if ( !baseInfo )
            {
                LOG_ERR( "Model %s not found in database", modelRenderer.model->name.c_str() );
            }
            else
            {
                auto modelInfo = std::static_pointer_cast<ModelCreateInfo>( baseInfo );
                //modelInfo->filename;
            }
        }
    });

    delete scene;
}


int main( int argc, char** argv )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    g_converterConfigOptions = {};
    
    std::string sceneFile;
    if ( !ParseCommandLineArgs( argc, argv, sceneFile ) ) return 0;

    AssetDatabase::Init();

    ConvertAssets( sceneFile );

    std::vector< std::string > assetsUsed[AssetType::NUM_ASSET_TYPES];
    FindAssetsUsedInScene( sceneFile, assetsUsed );
    
    const char* PG_ASSET_NAMES[] =
    {
        "Image",
        "Material",
        "Script",
        "Model",
        "Shader",
    };

    for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::NUM_ASSET_TYPES; ++assetTypeIdx )
    {
        for ( size_t assetIdx = 0; assetIdx < assetsUsed[assetTypeIdx].size(); ++assetIdx )
        {
            const std::string& assetName = assetsUsed[assetTypeIdx][assetIdx];

            LOG( "%s: %s", PG_ASSET_NAMES[assetTypeIdx], assetName.c_str() );

            //auto createInfoPtr = AssetDatabase::FindAssetInfo( (AssetType)assetTypeIdx, assetName );
            //if ( !createInfoPtr )
            //{
            //    LOG_ERR( "Scene requires asset %s of type %s, but no valid entry found in the database.", assetName.c_str(), PG_ASSET_NAMES[assetTypeIdx] );
            //    LOG_ERR( "Failed to convert scene %s", sceneFile.c_str() );
            //    return 0;
            //}

        }
    }
    

    Logger_Shutdown();
    return 0;
}