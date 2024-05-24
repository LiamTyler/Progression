#include "asset_file_database.hpp"
// #include "asset/parsing/base_asset_parse.hpp"
#include "asset/parsing/asset_parsers.hpp"
#include "core/feature_defines.hpp"
#include "core/time.hpp"
#include "shared/file_dependency.hpp"
#include "shared/filesystem.hpp"
#include <fstream>
#include <unordered_map>

namespace PG::AssetDatabase
{

static std::unordered_map<std::string, std::shared_ptr<BaseAssetCreateInfo>> s_assetInfos[AssetType::ASSET_TYPE_COUNT];

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
        const json::Value& value = assetIter->MemberBegin()->value;
        std::string assetName;
        if ( !value.HasMember( "name" ) )
        {
            LOG_ERR( "Asset '%s' in file '%s' needs to have a 'name' entry!", value.GetString(), filename.c_str() );
            return false;
        }
        assetName = value["name"].GetString();

        bool foundType           = false;
        std::string assetTypeStr = assetIter->MemberBegin()->name.GetString();
        for ( uint32_t typeIndex = 0; typeIndex < ASSET_TYPE_COUNT; ++typeIndex )
        {
            if ( assetTypeStr == g_assetNames[typeIndex] )
            {
                if ( s_assetInfos[typeIndex].find( assetName ) != s_assetInfos[typeIndex].end() )
                {
                    LOG_WARN( "Duplicate %s named %s in file %s. Ignoring", assetTypeStr.c_str(), assetName.c_str(), filename.c_str() );
                }
                else
                {
                    std::shared_ptr<BaseAssetCreateInfo> info;
                    info = g_assetParsers[typeIndex]->Parse( value );
                    if ( !info )
                    {
                        return false;
                    }
                    info->name                         = assetName;
                    s_assetInfos[typeIndex][assetName] = info;
                }

                foundType = true;
                break;
            }
        }

        if ( !foundType )
        {
            LOG_WARN( "Asset type '%s' not recognized. Skipping asset '%s'", assetTypeStr.c_str(), assetName.c_str() );
        }
    }

    return true;
}

bool Init()
{
    // Time::Point startTime = Time::GetTimePoint();

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

    // LOG( "Asset Database Initialized in %.2f seconds.", Time::GetTimeSince( startTime ) / 1000.0f );
    return true;
}

std::shared_ptr<BaseAssetCreateInfo> FindAssetInfo( AssetType type, const std::string& name )
{
    auto it = s_assetInfos[type].find( name );
    return it == s_assetInfos[type].end() ? nullptr : it->second;
}

} // namespace PG::AssetDatabase
