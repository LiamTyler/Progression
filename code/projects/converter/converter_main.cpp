#include "asset/asset_file_database.hpp"
#include "asset/asset_manager.hpp"
#include "asset/asset_versions.hpp"
#include "converters.hpp"
#include "core/init.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "ecs/components/model_renderer.hpp"
#include "getopt/getopt.h"
#include "shared/assert.hpp"
#include "shared/file_dependency.hpp"
#include "shared/filesystem.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "shared/string.hpp"
#include <algorithm>
#include <unordered_set>

using namespace PG;

static void DisplayHelp()
{
    auto msg =
        "Usage: converter [options] [ASSET_NAME or SCENE_FILE]\n"
        "SCENE_FILE is relative to the project's assets/ folder. Can also pass in a specific CSV instead of a regular json scene file\n"
        "ASSET_NAME is the name of the asset to be converted. Only applies when using --single, otherwise it's always interpreted as a "
        "scene path\n"
        "Options\n"
        "  --force        Don't check asset file dependencies, just reconvert everything\n"
        "  --help         Print this message and exit\n"
        "  --single       Used to process a specific asset (and its referenced assets). Format: type name. Ex: '--single material "
        "\n";

    LOG( "%s", msg );
}

static AssetType s_singleAssetType;
static std::string s_singleAssetName;
static uint32_t s_outOfDateScenes = 0;

static const std::string SCENE_DIR = PG_ASSET_DIR "scenes/";

static bool ParseCommandLineArgs( int argc, char** argv, std::string& sceneFile )
{
    if ( argc == 1 )
    {
        return true;
    }

    static struct option long_options[] =
    {
        { "force",  no_argument,       0, 'f' },
        { "help",   no_argument,       0, 'h' },
        { "single", required_argument, 0, 's' },
        { 0,        0,                 0, 0   }
    };

    s_singleAssetType = ASSET_TYPE_COUNT;
    int option_index  = 0;
    int c             = -1;
    while ( ( c = getopt_long( argc, argv, "fhs", long_options, &option_index ) ) != -1 )
    {
        switch ( c )
        {
        case 'f':
            g_converterConfigOptions.force = true;
            break;
        case 'h':
            DisplayHelp();
            return false;
        case 's':
            for ( uint32_t i = 0; i < Underlying( ASSET_TYPE_COUNT ); ++i )
            {
                if ( !Stricmp( g_assetNames[i], optarg ) )
                {
                    s_singleAssetType = (AssetType)i;
                }
            }
            if ( s_singleAssetType == ASSET_TYPE_COUNT )
            {
                LOG_ERR( "No asset type with name '%s'", optarg );
                return false;
            }
            break;
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
    if ( s_singleAssetType != ASSET_TYPE_COUNT )
    {
        s_singleAssetName = argv[optind];
    }
    else
    {
        sceneFile = argv[optind];
    }

    return true;
}

bool FindAssetsUsedInFile( const std::string& sceneFile )
{
    std::string ext = GetFileExtension( sceneFile );
    if ( ext == ".csv" )
    {
        std::string line;
        std::ifstream in( sceneFile );
        if ( !in )
        {
            LOG_ERR( "Could not open %s", sceneFile.c_str() );
            return false;
        }
        int lineIdx = -1;
        while ( std::getline( in, line ) )
        {
            ++lineIdx;
            line = StripWhitespace( line );
            if ( line.empty() || ( line.length() >= 2 && line[0] == '/' && line[1] == '/' ) )
                continue;

            const auto vec = SplitString( line );
            if ( vec.size() != 2 )
            {
                LOG_ERR( "Asset CSV %s: Invalid line %d '%s'", sceneFile.c_str(), lineIdx, line.c_str() );
                continue;
            }
            bool found                = false;
            uint32_t assetTypeIdx     = 0;
            std::string assetTypeName = vec[0];
            std::string assetName     = vec[1];
            for ( ; assetTypeIdx < ASSET_TYPE_COUNT; ++assetTypeIdx )
            {
                if ( assetTypeName == g_assetNames[assetTypeIdx] )
                {
                    auto createInfo = AssetDatabase::FindAssetInfo( (AssetType)assetTypeIdx, assetName );
                    if ( !createInfo )
                    {
                        LOG_ERR( "Could not find AssetInfo for AssetType: %s, name '%s'", assetTypeName.c_str(), assetName.c_str() );
                    }
                    g_converters[assetTypeIdx]->AddReferencedAssets( createInfo );
                    AddUsedAsset( (AssetType)assetTypeIdx, createInfo );
                    found = true;
                    break;
                }
            }
            if ( !found )
            {
                LOG_ERR( "Asset CSV %s: No asset type '%s' line %d", sceneFile.c_str(), vec[0].c_str(), lineIdx );
                return false;
            }
        }
    }
    else if ( ext == ".json" )
    {
        Scene* scene = Scene::Load( sceneFile );
        if ( !scene )
        {
            return false;
        }

        for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::ASSET_TYPE_COUNT; ++assetTypeIdx )
        {
            AssetType assetType = (AssetType)assetTypeIdx;
            for ( auto [assetName, _] : AssetManager::g_resourceMaps[assetTypeIdx] )
            {
                auto createInfo = AssetDatabase::FindAssetInfo( assetType, assetName );
                PG_ASSERT( createInfo, "asset with name %s not found", assetName.c_str() );
                AddUsedAsset( assetType, createInfo );
            }
        }

        delete scene;
    }
    else
    {
        LOG_ERR( "Unknown scene file extension for scene '%s'", sceneFile.c_str() );
        return false;
    }

    return true;
}

bool ConvertAssets( const std::string& sceneName, uint32_t& outOfDateAssets )
{
    auto convertStartTime  = Time::GetTimePoint();
    uint32_t convertErrors = 0;
    outOfDateAssets        = 0;
    uint32_t totalAssets   = 0;
    std::vector<std::pair<AssetType, std::shared_ptr<BaseAssetCreateInfo>>> outOfDateAssetList;
    outOfDateAssetList.reserve( 100 );
    for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::ASSET_TYPE_COUNT; ++assetTypeIdx )
    {
        const auto& pendingConvertList = GetUsedAssetsOfType( (AssetType)assetTypeIdx );
        for ( const std::shared_ptr<BaseAssetCreateInfo>& createInfo : pendingConvertList )
        {
            ++totalAssets;
            AssetStatus status = g_converters[assetTypeIdx]->IsAssetOutOfDate( createInfo );
            if ( status == AssetStatus::ERROR )
            {
                ++convertErrors;
                continue;
            }
            else if ( status == AssetStatus::OUT_OF_DATE )
            {
                ++outOfDateAssets;
                outOfDateAssetList.emplace_back( (AssetType)assetTypeIdx, createInfo );
            }
        }
    }

    if ( !convertErrors )
    {
        #pragma omp parallel for schedule( dynamic )
        for ( int i = 0; i < (int)outOfDateAssetList.size(); ++i )
        {
            uint32_t assetTypeIdx                                  = outOfDateAssetList[i].first;
            const std::shared_ptr<BaseAssetCreateInfo>& createInfo = outOfDateAssetList[i].second;
            if ( !g_converters[assetTypeIdx]->Convert( createInfo ) )
            {
                convertErrors += 1;
            }
        }
    }

    double duration = Time::GetTimeSince( convertStartTime ) / 1000.0f;
    if ( convertErrors )
    {
        LOG_ERR( "Convert for '%s' FAILED with %u errors in %.2f seconds", sceneName.c_str(), convertErrors, duration );
    }
    else
    {
        LOG( "Convert for '%s' succeeded in %.2f seconds, %u / %u assets out of date", sceneName.c_str(), duration, outOfDateAssets,
            totalAssets );
    }

    return convertErrors == 0;
}

bool OutputFastfile( const std::string& sceneFile, const uint32_t outOfDateAssets )
{
    std::string fastfileName = GetFilenameStem( sceneFile ) + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    std::string fastfilePath = PG_ASSET_DIR "cache/fastfiles/" + fastfileName;
    time_t ffTimestamp       = GetFileTimestamp( fastfilePath );

    bool createFastFile = false;
    if ( ffTimestamp == NO_TIMESTAMP )
    {
        LOG( "Fastfile %s is missing. Building...", fastfileName.c_str() );
        createFastFile = true;
    }
    else if ( ffTimestamp < GetLatestFastfileDependency() || outOfDateAssets )
    {
        LOG( "Fastfile %s is out of date. Rebuilding...", fastfileName.c_str() );
        createFastFile = true;
    }

    if ( createFastFile )
    {
        ++s_outOfDateScenes;
        Serializer ff;
        if ( !ff.OpenForWrite( fastfilePath ) )
        {
            LOG_ERR( "Could not open fastfile for writing" );
            return false;
        }

        for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::ASSET_TYPE_COUNT; ++assetTypeIdx )
        {
            const auto& listOfUsedAssets = GetUsedAssetsOfType( (AssetType)assetTypeIdx );
            for ( const auto& baseInfo : listOfUsedAssets )
            {
                ff.Write( assetTypeIdx );
                const std::string cacheName = g_converters[assetTypeIdx]->GetCacheName( baseInfo );
                size_t numBytes;
                auto assetRawBytes = AssetCache::GetCachedAssetRaw( (AssetType)assetTypeIdx, cacheName, numBytes );
                if ( !assetRawBytes )
                {
                    ff.Close();
                    DeleteFile( fastfilePath );
                    LOG_ERR( "Could not get cached asset %s of type %s", cacheName.c_str(), g_assetNames[assetTypeIdx] );
                    return false;
                }
                ff.Write( assetRawBytes.get(), numBytes );
            }
        }
        ff.Close();
        LOG( "Build fastfile succeeded" );
    }
    else
    {
        LOG( "Fastfile %s is up to date already!", fastfileName.c_str() );
    }

    return true;
}

bool ProcessSingleScene( const std::string& sceneFile )
{
    ClearAllFastfileDependencies();
    ClearAllUsedAssets();

    bool foundScene      = false;
    std::string filename = SCENE_DIR + sceneFile + ".csv";
    if ( PathExists( filename ) )
    {
        if ( !FindAssetsUsedInFile( filename ) )
        {
            return false;
        }
        AddFastfileDependency( filename );
        foundScene = true;
    }
    filename = SCENE_DIR + sceneFile + ".json";
    if ( PathExists( filename ) )
    {
        if ( !FindAssetsUsedInFile( filename ) )
        {
            return false;
        }
        AddFastfileDependency( filename );
        foundScene = true;
    }
    if ( !foundScene )
    {
        LOG_ERR( "No scene file (csv or json) found for path '%s'", sceneFile.c_str() );
        return false;
    }

    uint32_t outOfDateAssets;
    bool success = ConvertAssets( GetRelativeFilename( sceneFile ), outOfDateAssets );
    success      = success && OutputFastfile( sceneFile, outOfDateAssets );
    return success;
}

bool ConvertScenes( const std::string& scene )
{
    std::vector<std::string> scenesToProcess;
    if ( !scene.empty() )
    {
        scenesToProcess.push_back( GetFilenameMinusExtension( scene ) );
    }

    namespace fs = std::filesystem;
    for ( const auto& entry : fs::recursive_directory_iterator( PG_ASSET_DIR "scenes/required/" ) )
    {
        std::string path = entry.path().string();
        if ( entry.is_regular_file() && GetFileExtension( path ) == ".csv" )
        {
            scenesToProcess.push_back( "required/" + GetRelativeFilename( GetFilenameMinusExtension( path ) ) );
        }
    }
    std::swap( scenesToProcess[0], scenesToProcess[scenesToProcess.size() - 1] );

    for ( size_t i = 0; i < scenesToProcess.size(); ++i )
    {
        const auto& scene = scenesToProcess[i];
        LOG( "Converting %s...", GetRelativeFilename( scene ).c_str() );
        if ( !ProcessSingleScene( scene ) )
        {
            return false;
        }
        LOG( "" );
    }

    return true;
}

bool ConvertSingleAsset()
{
    auto createInfo = AssetDatabase::FindAssetInfo( s_singleAssetType, s_singleAssetName );
    if ( !createInfo )
    {
        LOG_ERR( "No asset of type '%s' found with name '%s'", g_assetNames[s_singleAssetType], s_singleAssetName.c_str() );
        return false;
    }

    g_converters[s_singleAssetType]->AddReferencedAssets( createInfo );
    AddUsedAsset( s_singleAssetType, createInfo );
    uint32_t outOfDateAssets;
    return ConvertAssets( s_singleAssetName, outOfDateAssets );
}

int main( int argc, char** argv )
{
    auto initStartTime = Time::GetTimePoint();
    EngineInitialize();
    g_converterConfigOptions = {};
    std::string sceneFile;
    s_singleAssetName = "";
    if ( !ParseCommandLineArgs( argc, argv, sceneFile ) )
    {
        return 0;
    }
    AssetCache::Init();
    InitConverters();
    if ( !AssetDatabase::Init() )
    {
        return 0;
    }
    LOG( "Converter initialized in %.2f seconds\n", Time::GetTimeSince( initStartTime ) / 1000.0f );

    if ( !s_singleAssetName.empty() )
    {
        if ( !ConvertSingleAsset() )
        {
            LOG_ERR( "Single asset convert failed" );
        }
    }
    else
    {
        if ( ConvertScenes( sceneFile ) )
        {
            if ( s_outOfDateScenes == 0 )
            {
                LOG( "All Scenes up to date already!" );
            }
        }
    }

    LOG( "Total time: %.2f seconds", Time::GetTimeSince( initStartTime ) / 1000.0f );

    ShutdownConverters();
    EngineShutdown();
    return 0;
}
