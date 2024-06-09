#include "base_asset_converter.hpp"
#include "asset/asset_manager.hpp"
#include "converters.hpp"
#include <unordered_set>

namespace PG
{

ConverterConfigOptions g_converterConfigOptions;
static time_t s_latestAssetTimestamp;

void AddFastfileDependency( const std::string& file )
{
    s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, GetFileTimestamp( file ) );
}

void AddFastfileDependency( time_t timestamp ) { s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, timestamp ); }

void ClearAllFastfileDependencies() { s_latestAssetTimestamp = 0; }

time_t GetLatestFastfileDependency() { return s_latestAssetTimestamp; }

struct BaseCreateInfoHash
{
    size_t operator()( const BaseCreateInfoPtr& ptr ) const { return std::hash<std::string>()( ptr->name ); }
};

struct BaseCreateInfoCompare
{
    size_t operator()( const BaseCreateInfoPtr& p1, const BaseCreateInfoPtr& p2 ) const { return p1->name == p2->name; }
};

static std::unordered_set<BaseCreateInfoPtr, BaseCreateInfoHash, BaseCreateInfoCompare> s_pendingAssets[AssetType::ASSET_TYPE_COUNT];

void ClearAllUsedAssets()
{
    for ( u32 assetTypeIdx = 0; assetTypeIdx < AssetType::ASSET_TYPE_COUNT; ++assetTypeIdx )
    {
        s_pendingAssets[assetTypeIdx].clear();
        AssetManager::g_resourceMaps[assetTypeIdx].clear();
    }
}

void AddUsedAsset( AssetType assetType, const BaseCreateInfoPtr& createInfo )
{
    if ( createInfo == nullptr )
    {
        LOG_ERR( "Adding null asset for conversion! AssetType: %s", g_assetNames[assetType] );
        throw std::runtime_error( "Adding null asset for conversion! AssetType: " + std::string( g_assetNames[assetType] ) );
    }

    s_pendingAssets[assetType].insert( createInfo );
    g_converters[assetType]->AddReferencedAssets( createInfo );
}

std::vector<BaseCreateInfoPtr> GetUsedAssetsOfType( AssetType assetType )
{
    return { s_pendingAssets[assetType].begin(), s_pendingAssets[assetType].end() };
}

} // namespace PG
