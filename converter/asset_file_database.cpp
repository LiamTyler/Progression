#include "asset_file_database.hpp"
#include "converters/base_asset_converter.hpp"
#include "converters/gfx_image_converter.hpp"
#include "converters/material_converter.hpp"
#include "converters/model_converter.hpp"
#include "converters/script_converter.hpp"
#include "converters/shader_converter.hpp"
#include "core/platform_defines.hpp"
#include "core/time.hpp"
#include "utils/filesystem.hpp"
#include <unordered_map>
#include <filesystem>


namespace PG::AssetDatabase
{

static std::unordered_map< std::string, std::shared_ptr<BaseAssetCreateInfo> > s_assetInfos[AssetType::NUM_ASSET_TYPES];


static bool ParseAssetFile( const std::string& filename )
{
    static std::vector< std::shared_ptr<BaseAssetConverter> > converters =
    {
        std::make_shared<GfxImageConverter>(),
        std::make_shared<MaterialConverter>(),
        std::make_shared<ScriptConverter>(),
        std::make_shared<ModelConverter>(),
        std::make_shared<ShaderConverter>(),
    };

    namespace json = rapidjson;
    json::Document document;
    if ( !ParseJSONFile( filename, document ) )
    {
        LOG_ERR( "Skipping asset file %s", filename.c_str() );
        return false;
    }
    for ( json::Value::ConstValueIterator assetIter = document.Begin(); assetIter != document.End(); ++assetIter )
    {
        std::string assetTypeStr = assetIter->MemberBegin()->name.GetString();
        const json::Value& value = assetIter->MemberBegin()->value;
        for ( int typeIndex = 0; typeIndex < converters.size(); ++typeIndex )
        {
            if ( assetTypeStr == converters[typeIndex]->assetNameInJsonFile )
            {
                const std::string assetName = value["name"].GetString();
                if ( s_assetInfos[typeIndex].find( assetName ) != s_assetInfos[typeIndex].end() )
                {
                    LOG_ERR( "Duplicate %s name %s in file %s" );
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
                    auto createInfo = converters[typeIndex]->Parse( value, parent );
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