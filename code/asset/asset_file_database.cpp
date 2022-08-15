#include "asset_file_database.hpp"
//#include "asset/parsing/base_asset_parse.hpp"
#include "asset/parsing/asset_parsers.hpp"
#include "core/time.hpp"
#include "shared/filesystem.hpp"
#include "shared/file_dependency.hpp"
#include "shared/platform_defines.hpp"
#include <fstream>
#include <unordered_map>


namespace PG::AssetDatabase
{

//struct AssetInfo
//{
//    std::shared_ptr<BaseAssetCreateInfo> createInfo;
//};
//static std::unordered_map<std::string, std::shared_ptr<BaseAssetCreateInfo>> s_previousAssetInfo[AssetType::NUM_ASSET_TYPES];

static std::unordered_map<std::string, std::shared_ptr<BaseAssetCreateInfo>> s_assetInfos[AssetType::NUM_ASSET_TYPES];


static bool ParseAssetFile( const std::string& filename )
{
    namespace json = rapidjson;
    json::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        return false;
    }
    time_t fileTimestamp = GetFileTimestamp( filename );

    for ( json::Value::ConstValueIterator assetIter = document.Begin(); assetIter != document.End(); ++assetIter )
    {
        std::string assetTypeStr = assetIter->MemberBegin()->name.GetString();
        const json::Value& value = assetIter->MemberBegin()->value;
        for ( int typeIndex = 0; typeIndex < NUM_ASSET_TYPES; ++typeIndex )
        {
            if ( assetTypeStr == g_assetNames[typeIndex] )
            {
                const std::string assetName = value["name"].GetString();
                if ( s_assetInfos[typeIndex].find( assetName ) != s_assetInfos[typeIndex].end() )
                {
                    LOG_WARN( "Duplicate %s name %s in file %s. Ignoring", assetTypeStr.c_str(), assetName.c_str(), filename.c_str() );
                }
                else
                {
                    std::shared_ptr<BaseAssetCreateInfo> info;
                    info = g_assetParsers[typeIndex]->Parse( value );
                    if ( !info )
                    {
                        return false;
                    }
                    info->name = assetName;
                    s_assetInfos[typeIndex][assetName] = info;
                }

                break;
            }
        }
    }

    return true;
}

/*
void TryLoadCache()
{
    std::ifstream in( PG_ASSET_DIR "cache/paf_cache.bin", std::ios::binary );
    if ( !in ) return;

    for ( int typeIndex = 0; typeIndex < NUM_ASSET_TYPES; ++typeIndex )
    {
        uint32_t numAssets;
        in >> numAssets;
        s_previousAssetMetadata[typeIndex].reserve( numAssets );

        AssetMetadata metadata;
        std::string name;
        uint32_t strLen;
        in >> strLen;
        name.resize( strLen );
        in.read( &name[0], strLen );
        in >> metadata.hash;
        in >> metadata.timestamp;

        s_previousAssetMetadata[typeIndex][name] = metadata;
    }

}
*/

bool Init()
{
    Time::Point startTime = Time::GetTimePoint();
    //TryLoadCache();

    {
        namespace fs = std::filesystem;
        for ( const auto& entry : fs::recursive_directory_iterator( PG_ASSET_DIR ) )
        {
            if ( entry.is_regular_file() && GetFileExtension( entry.path().string() ) == ".paf" )
            {
                if ( !ParseAssetFile( entry.path().string() ) )
                {
                    LOG_ERR( "Failed to initialize asset database" );
                    return false;
                }
            }
        }
    }

    LOG( "Asset Database Initialized in %.2f seconds.", Time::GetDuration( startTime ) / 1000.0f );
    return true;
}


std::shared_ptr<BaseAssetCreateInfo> FindAssetInfo( AssetType type, const std::string& name )
{
    auto it = s_assetInfos[type].find( name );
    return it == s_assetInfos[type].end() ? nullptr : it->second;
}

} // namespace PG::AssetDatabase