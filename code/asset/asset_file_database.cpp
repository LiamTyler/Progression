#include "asset_file_database.hpp"
#include "asset/parsing/base_asset_parse.hpp"
#include "core/time.hpp"
#include "shared/filesystem.hpp"
#include "shared/platform_defines.hpp"
#include <unordered_map>


namespace PG::AssetDatabase
{

static std::unordered_map< std::string, std::shared_ptr<BaseAssetCreateInfo> > s_assetInfos[AssetType::NUM_ASSET_TYPES];


static bool ParseAssetFile( const std::string& filename )
{
    namespace json = rapidjson;
    json::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        LOG_WARN( "Skipping asset file %s", filename.c_str() );
        return false;
    }
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
                    LOG_ERR( "Duplicate %s name %s in file %s", assetTypeStr.c_str(), assetName.c_str(), filename.c_str() );
                }
                else
                {
                    std::shared_ptr<BaseAssetCreateInfo> parent = nullptr;
                    if ( value.HasMember( "parent" ) )
                    {
                        auto it = s_assetInfos[typeIndex].find( value["parent"].GetString() );
                        if ( it == s_assetInfos[typeIndex].end() )
                        {
                            LOG_ERR( "Parent %s for asset %s does not exist! Must be declared in same asset file to inherit.", value["parent"].GetString(), assetName.c_str() );
                        }
                        else
                        {
                            parent = it->second;
                        }
                    }
                    auto createInfo = g_assetParsers[typeIndex]->Parse( value, parent );
                    if ( createInfo )
                    {
                        createInfo->name = assetName;
                        s_assetInfos[typeIndex][assetName] = createInfo;
                    }
                }

                break;
            }
        }
    }

    return true;
}


void Init()
{
    Time::Point startTime = Time::GetTimePoint();

    namespace fs = std::filesystem;
    for ( const auto& entry : fs::recursive_directory_iterator( PG_ASSET_DIR ) )
    {
        if ( entry.is_regular_file() && GetFileExtension( entry.path().string() ) == ".paf" )
        {
            ParseAssetFile( entry.path().string() );
        }
    }

    LOG( "Asset Database Initialized in %.2f seconds.", Time::GetDuration( startTime ) / 1000.0f );
}


std::shared_ptr<BaseAssetCreateInfo> FindAssetInfo( AssetType type, const std::string& name )
{
    auto it = s_assetInfos[type].find( name );
    return it == s_assetInfos[type].end() ? nullptr : it->second;
}

} // namespace PG::AssetDatabase