#include "asset/asset_manager.hpp"
#include "asset/asset_versions.hpp"
#include "asset_file_database.hpp"
#include "converters.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "getopt/getopt.h"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/file_dependency.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <algorithm>
#include <unordered_set>

using namespace PG;

static void DisplayHelp()
{
    auto msg =
      "Usage: converter [options] SCENE_FILE_OR_CSV\n"
      "SCENE_FILE is relative to the project's assets/ folder. Can also pass in a specific CSV instead of a regular json scene file\n";
      "Options\n"
      "  --force            Don't check asset file dependencies, just reconvert everything\n"
      "  --help             Print this message and exit\n";
      "  --single           Only process this scene, and not any of the dependent scenes. Assumed if CSV is passed in\n";

    LOG( "%s", msg );
}


static bool ParseCommandLineArgs( int argc, char** argv, std::string& sceneFile, bool& single )
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
        { "single",               no_argument,  0, 's' },
        { 0, 0, 0, 0 }
    };

    single = false;
    int option_index = 0;
    int c            = -1;
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
                single = true;
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
    sceneFile = PG_ASSET_DIR + std::string( argv[optind] );
    single = single || GetFileExtension( sceneFile ) == ".csv";
    return true;
}


static std::string StripWhitespace( const std::string& s )
{
    size_t start, end;
    for ( start = 0; start < s.length() && std::isspace( s[start] ); ++start );
    for ( end = s.length() - 1; end >= 0 && std::isspace( s[end] ); --end );

    return s.substr( start, end - start + 1 );
}


static std::vector<std::string> SplitString( const std::string& str, const std::string delim = "," )
{
	if ( str.empty() )
		return {};

	std::vector<std::string> ret;
	size_t start_index = 0;
	size_t index = 0;
	while ( (index = str.find_first_of(delim, start_index) ) != std::string::npos )
	{
		ret.push_back(str.substr(start_index, index - start_index));
		start_index = index + 1;

		if ( index == str.size() - 1 )
        {
			ret.emplace_back();
        }
	}

	if ( start_index < str.size() )
    {
		ret.push_back( str.substr(start_index) );
    }
	return ret;
}


bool FindAssetsUsedInFile( const std::string& sceneFile, std::unordered_set<std::string> assetsUsed[AssetType::NUM_ASSET_TYPES] )
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
            if ( line.empty() || (line.length() >= 2 && line[0] == '/' && line[1] == '/' ) )
                continue;

            const auto vec = SplitString( line );
            if ( vec.size() != 2 )
            {
                LOG_ERR( "Asset CSV %s: Invalid line %d '%s'", sceneFile.c_str(), lineIdx, line.c_str() );
                continue;
            }
            bool found = false;
            for ( int i = 0; i < NUM_ASSET_TYPES; ++i )
            {
                if ( vec[0] == g_converters[i]->assetNameInJsonFile )
                {
                    assetsUsed[i].insert( vec[1] );
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
                    delete scene;
                    return false;
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
                delete scene;
                return false;
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
    else
    {
        LOG_ERR( "Unknown scene file extension for scene '%s'", sceneFile.c_str() );
        return false;
    }

    return true;
}


bool ConvertAssets( const std::string& sceneName, uint32_t& outOfDateAssets, const std::unordered_set<std::string> (&assetsUsed)[AssetType::NUM_ASSET_TYPES] )
{
    auto convertStartTime = Time::GetTimePoint();
    uint32_t convertErrors = 0;
    outOfDateAssets = 0;
    for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::NUM_ASSET_TYPES; ++assetTypeIdx )
    {
        for ( const auto& assetName : assetsUsed[assetTypeIdx] )
        {
            AssetStatus status = g_converters[assetTypeIdx]->IsAssetOutOfDate( assetName );
            if ( status == AssetStatus::ERROR )
            {
                ++convertErrors;
                continue;
            }
            else if ( status == AssetStatus::OUT_OF_DATE )
            {
                ++outOfDateAssets;
                if ( !g_converters[assetTypeIdx]->Convert( assetName ) )
                {
                    convertErrors += 1;
                }
            }
        }
    }

    double duration = Time::GetDuration( convertStartTime ) / 1000.0f;
    if ( convertErrors )
    {
        LOG_ERR( "Convert %s FAILED with %u errors in %.2f seconds", sceneName.c_str(), convertErrors, duration );
    }
    else
    {
        LOG( "Convert %s succeeded in %.2f seconds", sceneName.c_str(), duration );
    }

    return convertErrors == 0;
}


bool OutputFastfile( const std::string& sceneFile, const uint32_t outOfDateAssets, const std::unordered_set<std::string> (&assetsUsed)[AssetType::NUM_ASSET_TYPES] )
{
    std::string fastfileName = GetFilenameStem( sceneFile ) + "_v" + std::to_string( PG_FASTFILE_VERSION ) + ".ff";
    std::string fastfilePath = PG_ASSET_DIR "cache/fastfiles/" + fastfileName;
    time_t ffTimestamp = GetFileTimestamp( fastfilePath );
    if ( ffTimestamp == NO_TIMESTAMP || ffTimestamp < GetLatestFastfileDependency() )
    {
        LOG( "Fastfile %s is out of date. Rebuilding...", fastfileName.c_str() );
        auto startTime = Time::GetTimePoint();
        Serializer ff;
        if ( !ff.OpenForWrite( fastfilePath ) )
        {
            LOG_ERR( "Could not open fastfile for writing" );
            return false;
        }

        for ( unsigned int assetTypeIdx = 0; assetTypeIdx < AssetType::NUM_ASSET_TYPES; ++assetTypeIdx )
        {
            for ( const auto& assetName : assetsUsed[assetTypeIdx] )
            {
                ff.Write( assetTypeIdx );
                auto baseInfo = AssetDatabase::FindAssetInfo( (AssetType)assetTypeIdx, assetName );
                const std::string cacheName = g_converters[assetTypeIdx]->GetCacheName( baseInfo );
                size_t numBytes;
                auto assetRawBytes = AssetCache::GetCachedAssetRaw( (AssetType)assetTypeIdx, cacheName, numBytes );
                if ( !assetRawBytes )
                {
                    ff.Close();
                    DeleteFile( fastfilePath );
                    LOG_ERR( "Could not get cached asset %s", cacheName.c_str() );
                    return false;
                }
                ff.Write( assetRawBytes.get(), numBytes );
            }
        }
        ff.Close();
        LOG( "Built fastfile in %.2f seconds", Time::GetDuration( startTime ) / 1000.0f );
    }
    else
    {
        PG_ASSERT( outOfDateAssets == 0 );
        LOG( "Fastfile %s is up to date already!", fastfileName.c_str() );
    }

    return true;
}


bool ProcessScene( const std::string& sceneFile )
{
    ClearAllFastfileDependencies();

    //auto enumerateStartTime = Time::GetTimePoint();
    std::unordered_set<std::string> assetsUsed[AssetType::NUM_ASSET_TYPES];
    if ( PathExists( sceneFile + ".csv" ) )
    {
        if ( !FindAssetsUsedInFile( sceneFile + ".csv", assetsUsed ) )
        {
            return false;
        }
        AddFastfileDependency( sceneFile + ".csv" );
    }
    if ( PathExists( sceneFile + ".json" ) )
    {
        if ( !FindAssetsUsedInFile( sceneFile + ".json", assetsUsed ) )
        {
            return false;
        }
        AddFastfileDependency( sceneFile + ".json" );
    }
    //LOG( "Assets for scene %s enumerated in %.2f seconds", GetRelativeFilename( sceneFile ).c_str(), Time::GetDuration( enumerateStartTime ) / 1000.0f );

    uint32_t outOfDateAssets;
    bool success = ConvertAssets( GetRelativeFilename( sceneFile ), outOfDateAssets, assetsUsed );
    success = success && OutputFastfile( sceneFile, outOfDateAssets, assetsUsed );
    return success;
}


int main( int argc, char** argv )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    auto initStartTime = Time::GetTimePoint();
    g_converterConfigOptions = {};
    std::string sceneFile;
    bool singleScene;
    if ( !ParseCommandLineArgs( argc, argv, sceneFile, singleScene ) )
    {
        return 0;
    }
    AssetManager::Init();
    AssetCache::Init();
    InitConverters();
    AssetDatabase::Init();
    LOG( "Converter initialized in %.2f seconds", Time::GetDuration( initStartTime ) / 1000.0f );

    std::vector<std::string> scenesToProcess;
    scenesToProcess.push_back( GetFilenameMinusExtension( sceneFile ) );
    if ( !singleScene )
    {
        namespace fs = std::filesystem;
        for ( const auto& entry : fs::recursive_directory_iterator( PG_ASSET_DIR "scenes/required/" ) )
        {
            std::string path = entry.path().string();
            if ( entry.is_regular_file() && GetFileExtension( path ) == ".csv" )
            {
                scenesToProcess.push_back( GetFilenameMinusExtension( path ) );
            }
        }
    }

    LOG( "" );
    for ( size_t i = 0; i < scenesToProcess.size(); ++i )
    {
        const auto& scene = scenesToProcess[i];
        LOG( "Converting %s...", GetRelativeFilename( scene ).c_str() );
        if ( !ProcessScene( scene ) )
        {
            break;
        }
        LOG( "" );
    }
    
    ShutdownConverters();
    Logger_Shutdown();
    return 0;
}