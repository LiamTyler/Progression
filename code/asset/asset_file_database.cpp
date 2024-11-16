#include "asset_file_database.hpp"
#include "asset/parsing/asset_parsers.hpp"
#include "core/feature_defines.hpp"
#include "core/time.hpp"
#include "shared/file_dependency.hpp"
#include "shared/filesystem.hpp"
#include <fstream>
#include <unordered_map>

namespace PG::AssetDatabase
{

static std::unordered_map<std::string, std::shared_ptr<BaseAssetCreateInfo>> s_assetInfos[ASSET_TYPE_COUNT];

static bool IsNameValid( const std::string& name )
{
    for ( size_t i = 0; i < name.length(); ++i )
    {
        char c = name[i];
        if ( !isalnum( c ) && c != '_' )
            return false;
    }

    return true;
}

static bool ParseAssetFile( const std::string& filename )
{
    namespace json = rapidjson;
    json::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        return false;
    }

    for ( json::Value::ConstValueIterator assetIter = document.Begin(); assetIter != document.End(); ++assetIter )
    {
        const json::Value& value = assetIter->MemberBegin()->value;
        if ( !value.HasMember( "name" ) )
        {
            LOG_ERR( "Asset '%s' in file '%s' needs to have a 'name' entry!", value.GetString(), filename.c_str() );
            return false;
        }
        const auto& jsonName = value["name"];
        std::string assetName( jsonName.GetString(), jsonName.GetStringLength() );
        if ( !IsNameValid( assetName ) )
        {
            LOG_ERR( "Asset '%s' in file '%s' has an invalid name. Only letters, numbers, and '_' are allowed!", assetName.c_str(),
                filename.c_str() );
            return false;
        }
        bool isDebugOnlyAsset = value.HasMember( "isDebugOnlyAsset" ) ? value["isDebugOnlyAsset"].GetBool() : false;

        bool foundType           = false;
        std::string assetTypeStr = assetIter->MemberBegin()->name.GetString();
        for ( AssetType assetType = (AssetType)0; assetType < ASSET_TYPE_COUNT; assetType = (AssetType)( assetType + 1 ) )
        {
            if ( assetTypeStr == g_assetNames[assetType] )
            {
                if ( s_assetInfos[assetType].contains( assetName ) )
                {
                    LOG_WARN( "Duplicate %s named %s in file %s. Ignoring", assetTypeStr.c_str(), assetName.c_str(), filename.c_str() );
                }
                else
                {
                    std::shared_ptr<BaseAssetCreateInfo> parentCreateInfo = nullptr;
                    if ( value.HasMember( "parent" ) )
                        parentCreateInfo = FindAssetInfo( assetType, value["parent"].GetString() );

                    std::shared_ptr<BaseAssetCreateInfo> info;
                    info = g_assetParsers[assetType]->Parse( value, parentCreateInfo );
                    if ( !info )
                    {
                        return false;
                    }
#if USING( CONVERTER )
                    info->isDebugOnlyAsset = isDebugOnlyAsset;
#endif // #if USING( CONVERTER )
                    s_assetInfos[assetType][assetName] = info;
                }

                foundType = true;
                break;
            }
        }

        if ( !foundType )
        {
            bool skip = assetTypeStr.length() >= 2 && assetTypeStr[0] == '_' && assetTypeStr[1] == '_';
            if ( !skip )
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
