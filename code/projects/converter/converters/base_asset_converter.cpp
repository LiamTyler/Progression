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

void AssetList::Add( AssetType type, const std::string& str ) { assets[Underlying( type )].push_back( str ); }

void AssetList::Clear()
{
    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
    {
        assets[i].clear();
    }
}

void AssetList::Sort()
{
    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
    {
        std::sort( assets[i].begin(), assets[i].end() );
    }
}

void AssetList::Import( const std::string& sceneFile )
{
    std::ifstream in( PG_ASSET_DIR "cache/assetlists/" + sceneFile + ".txt" );
    if ( !in )
        return;

    u32 type;
    std::string name;
    while ( in >> type )
    {
        in >> name;
        Add( (AssetType)type, name );
    }
}

void AssetList::Export( const std::string& sceneFile ) const
{
    std::string filename = PG_ASSET_DIR "cache/assetlists/" + sceneFile + ".txt";
    CreateDirectory( GetParentPath( filename ) );
    std::ofstream out( filename );
    if ( !out )
    {
        LOG_ERR( "Failed to export asset list file %s", sceneFile.c_str() );
        return;
    }

    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
    {
        for ( const std::string& assetName : assets[i] )
            out << i << " " << assetName << "\n";
    }
}

bool AssetList::operator==( const AssetList& list ) const
{
    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
    {
        if ( assets[i] != list.assets[i] )
            return false;
    }

    return true;
}

struct BaseCreateInfoHash
{
    size_t operator()( const BaseCreateInfoPtr& ptr ) const { return std::hash<std::string>()( ptr->name ); }
};

struct BaseCreateInfoCompare
{
    size_t operator()( const BaseCreateInfoPtr& p1, const BaseCreateInfoPtr& p2 ) const { return p1->name == p2->name; }
};

static std::unordered_set<BaseCreateInfoPtr, BaseCreateInfoHash, BaseCreateInfoCompare> s_pendingAssets[ASSET_TYPE_COUNT];

void ClearAllUsedAssets()
{
    for ( u32 assetTypeIdx = 0; assetTypeIdx < ASSET_TYPE_COUNT; ++assetTypeIdx )
    {
        s_pendingAssets[assetTypeIdx].clear();
        AssetManager::g_resourceMaps[assetTypeIdx].clear();
    }
}

void AddUsedAsset( AssetType assetType, BaseCreateInfoPtr createInfo )
{
    if ( createInfo == nullptr )
    {
        LOG_ERR( "Adding null asset for conversion! AssetType: %s", g_assetNames[assetType] );
        throw std::runtime_error( "Adding null asset for conversion! AssetType: " + std::string( g_assetNames[assetType] ) );
    }
    createInfo->cacheName = g_converters[assetType]->GetCacheName( createInfo );

    s_pendingAssets[assetType].insert( createInfo );
    g_converters[assetType]->AddReferencedAssets( createInfo );
}

AssetList GetUsedAssetList()
{
    AssetList assetList;
    for ( u32 i = 0; i < ASSET_TYPE_COUNT; ++i )
    {
        const auto& assetCIMap = s_pendingAssets[i];
        assetList.assets->reserve( assetCIMap.size() );
        for ( const BaseCreateInfoPtr& info : assetCIMap )
        {
            assetList.Add( (AssetType)i, info->cacheName );
        }
    }

    assetList.Sort();
    return assetList;
}

std::vector<BaseCreateInfoPtr> GetUsedAssetsOfType( AssetType assetType )
{
    return { s_pendingAssets[assetType].begin(), s_pendingAssets[assetType].end() };
}

} // namespace PG
