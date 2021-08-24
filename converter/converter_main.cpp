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
#include <unordered_set>

using namespace PG;

static void DisplayHelp()
{
    auto msg =
      "Usage: converter [options] SCENE_FILE\n"
      "SCENE_FILE is relative to the project's assets/ folder\n";
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


bool ConvertAssets( const std::string& sceneFile )
{
    ClearAllFastfileDependencies();
    g_converterStatus       = {};
    auto convertAssetsStartTime = Time::GetTimePoint();

    return true;
}


void FindAssetsUsedInScene( const std::string& sceneFile, std::unordered_set<std::string> assetsUsed[AssetType::NUM_ASSET_TYPES] )
{
    Scene* scene = Scene::Load( sceneFile );

    for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::NUM_ASSET_TYPES; ++assetTypeIdx )
    {
        assetsUsed[assetTypeIdx].reserve( AssetManager::g_resourceMaps[assetTypeIdx].size() );
        for ( auto [assetName, _] : AssetManager::g_resourceMaps[assetTypeIdx] )
        {
            assetsUsed[assetTypeIdx].insert( assetName );
        }
    }

    for ( const auto [assetName, _] : AssetManager::g_resourceMaps[ASSET_TYPE_MODEL] )
    {
        auto modelInfo = std::static_pointer_cast<ModelCreateInfo>( AssetDatabase::FindAssetInfo( ASSET_TYPE_MODEL, assetName ) );
        if ( !modelInfo )
        {
            LOG_ERR( "Model %s not found in database", assetName.c_str() );
        }
        else
        {
            std::ifstream in( modelInfo->filename );
            if ( !in )
            {
                LOG_ERR( "Could not open model file %s", modelInfo->filename.c_str() );
                continue;
            }

            std::string line, matName, tmp;
            std::getline( in, line );
            std::stringstream ss( line );
            int numMaterials;
            ss >> tmp >> numMaterials;
            for ( int i = 0; i < numMaterials; ++i )
            {
                std::getline( in, line );
                ss = std::stringstream( line );
                ss >> tmp >> matName;
                assetsUsed[ASSET_TYPE_MATERIAL].insert( matName );
            }
        }
    }

    for ( const std::string& matName : assetsUsed[ASSET_TYPE_MATERIAL] )
    {
        auto info = std::static_pointer_cast<MaterialCreateInfo>( AssetDatabase::FindAssetInfo( ASSET_TYPE_MATERIAL, matName ) );
        if ( !info )
        {
            LOG_ERR( "Material %s not found in database", matName.c_str() );
        }
        else
        {
            if ( !info->albedoMapName.empty() ) assetsUsed[ASSET_TYPE_GFX_IMAGE].insert( info->albedoMapName );
            if ( !info->metalnessMapName.empty() ) assetsUsed[ASSET_TYPE_GFX_IMAGE].insert( info->metalnessMapName );
            if ( !info->roughnessMapName.empty() ) assetsUsed[ASSET_TYPE_GFX_IMAGE].insert( info->roughnessMapName );
        }
    }

    delete scene;
}


int main( int argc, char** argv )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    AssetManager::Init();

    g_converterConfigOptions = {};
    
    std::string sceneFile;
    if ( !ParseCommandLineArgs( argc, argv, sceneFile ) ) return 0;

    AssetDatabase::Init();

    std::unordered_set<std::string> assetsUsed[AssetType::NUM_ASSET_TYPES];
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
        for ( const auto& asset : assetsUsed[assetTypeIdx] )
        {
            LOG( "%s: %s", PG_ASSET_NAMES[assetTypeIdx], asset.c_str() );

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