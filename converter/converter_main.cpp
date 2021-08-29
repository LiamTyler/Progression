#include "asset/asset_versions.hpp"
#include "asset/asset_manager.hpp"
#include "asset_file_database.hpp"
#include "converters.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "getopt/getopt.h"
#include "utils/cpu_profile.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/logger.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"
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

            std::string matName, tmp;
            int numMaterials;
            in >> tmp >> numMaterials;
            for ( int i = 0; i < numMaterials; ++i )
            {
                in >> tmp >> matName;
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

    auto initStartTime = Time::GetTimePoint();
    g_converterConfigOptions = {};
    std::string sceneFile;
    if ( !ParseCommandLineArgs( argc, argv, sceneFile ) )
    {
        return 0;
    }
    AssetManager::Init();
    InitConverters();
    AssetDatabase::Init();
    LOG( "Converter initialized in %.2f seconds", Time::GetDuration( initStartTime ) / 1000.0f );

    auto enumerateStartTime = Time::GetTimePoint();
    std::unordered_set<std::string> assetsUsed[AssetType::NUM_ASSET_TYPES];
    FindAssetsUsedInScene( sceneFile, assetsUsed );
    LOG( "Scene assets enumerated in %.2f seconds", Time::GetDuration( enumerateStartTime ) / 1000.0f );

    auto convertStartTime = Time::GetTimePoint();
    ClearAllFastfileDependencies();
    uint32_t convertErrors = 0;
    for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::NUM_ASSET_TYPES; ++assetTypeIdx )
    {
        for ( const auto& asset : assetsUsed[assetTypeIdx] )
        {
            if ( !g_converters[assetTypeIdx]->Convert( asset ) )
            {
                convertErrors += 1;
            }
        }
    }

    double duration = Time::GetDuration( convertStartTime ) / 1000.0f;
    if ( convertErrors )
    {
        LOG_ERR( "Convert FAILED with %u errors in %.2f seconds", convertErrors, duration );
    }
    else
    {
        LOG( "Convert SUCCEEDED in %.2f seconds", duration );
    }
    
    ShutdownConverters();
    Logger_Shutdown();
    return 0;
}