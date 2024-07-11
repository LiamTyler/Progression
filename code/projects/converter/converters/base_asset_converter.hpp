#pragma once

#include "asset/asset_cache.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include "shared/assert.hpp"
#include "shared/file_dependency.hpp"
#include "shared/filesystem.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"

namespace PG
{

struct ConverterConfigOptions
{
    bool force             = false;
    bool saveShaderPreproc = false;
};

enum class AssetStatus : u8
{
    OUT_OF_DATE,
    UP_TO_DATE,
    ERROR
};

extern ConverterConfigOptions g_converterConfigOptions;

void AddFastfileDependency( const std::string& file );
void AddFastfileDependency( time_t timestamp );
void ClearAllFastfileDependencies();
time_t GetLatestFastfileDependency();

struct AssetList
{
    void Add( AssetType type, const std::string& str );
    void Clear();
    void Sort();
    void Import( const std::string& sceneFile );
    void Export( const std::string& sceneFile ) const;
    bool operator==( const AssetList& list ) const;

    // cache names
    std::vector<std::string> assets[ASSET_TYPE_COUNT];
};

using BaseCreateInfoPtr      = std::shared_ptr<BaseAssetCreateInfo>;
using ConstBaseCreateInfoPtr = const std::shared_ptr<const BaseAssetCreateInfo>;
void ClearAllUsedAssets();
void AddUsedAsset( AssetType assetType, BaseCreateInfoPtr createInfo );
AssetList GetUsedAssetList();
std::vector<BaseCreateInfoPtr> GetUsedAssetsOfType( AssetType assetType );

class BaseAssetConverter
{
public:
    const AssetType assetType;

    BaseAssetConverter( AssetType inAssetType ) : assetType( inAssetType ) {}
    virtual ~BaseAssetConverter() = default;

    virtual std::string GetCacheName( ConstBaseCreateInfoPtr& baseInfo ) { return ""; }
    virtual AssetStatus IsAssetOutOfDate( ConstBaseCreateInfoPtr& baseInfo ) { return AssetStatus::UP_TO_DATE; }
    virtual bool Convert( ConstBaseCreateInfoPtr& baseInfo ) { return true; }
    virtual void AddReferencedAssets( ConstBaseCreateInfoPtr& baseInfo ) {}
};

template <typename DerivedAsset, typename DerivedInfo>
class BaseAssetConverterTemplate : public BaseAssetConverter
{
public:
    using DerivedInfoPtr      = std::shared_ptr<DerivedInfo>;
    using ConstDerivedInfoPtr = const std::shared_ptr<const DerivedInfo>&;

    BaseAssetConverterTemplate( AssetType inAssetType ) : BaseAssetConverter( inAssetType ) {}
    virtual ~BaseAssetConverterTemplate() = default;

    virtual std::string GetCacheName( ConstBaseCreateInfoPtr& baseInfo )
    {
        return GetCacheNameInternal( std::static_pointer_cast<const DerivedInfo>( baseInfo ) ) + "_v" +
               std::to_string( g_assetVersions[assetType] );
    }

    virtual AssetStatus IsAssetOutOfDate( ConstBaseCreateInfoPtr& baseInfo ) override
    {
        if ( g_converterConfigOptions.force )
        {
            AddFastfileDependency( LATEST_TIMESTAMP );
            return AssetStatus::OUT_OF_DATE;
        }

        time_t cachedTimestamp = AssetCache::GetAssetTimestamp( assetType, baseInfo->cacheName );
        if ( cachedTimestamp == NO_TIMESTAMP )
        {
            AddFastfileDependency( LATEST_TIMESTAMP );
            return AssetStatus::OUT_OF_DATE;
        }
        AddFastfileDependency( cachedTimestamp );

        return IsAssetOutOfDateInternal( std::static_pointer_cast<const DerivedInfo>( baseInfo ), cachedTimestamp );
    }

    virtual bool Convert( ConstBaseCreateInfoPtr& baseInfo ) override
    {
        LOG( "Converting out of date asset %s %s...", g_assetNames[assetType], baseInfo->name.c_str() );
        return ConvertInternal( std::static_pointer_cast<const DerivedInfo>( baseInfo ) );
    }

    virtual void AddReferencedAssets( ConstBaseCreateInfoPtr& baseInfo ) override
    {
        AddReferencedAssetsInternal( std::static_pointer_cast<const DerivedInfo>( baseInfo ) );
    }

protected:
    virtual std::string GetCacheNameInternal( ConstDerivedInfoPtr derivedCreateInfo )                            = 0;
    virtual AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr derivedCreateInfo, time_t cacheTimestamp ) = 0;
    virtual void AddReferencedAssetsInternal( ConstDerivedInfoPtr& derivedCreateInfo ) {}

    virtual bool ConvertInternal( ConstDerivedInfoPtr& derivedCreateInfo )
    {
        DerivedAsset asset;
        const std::string& cacheName = derivedCreateInfo->cacheName;
        if ( !asset.Load( derivedCreateInfo.get() ) )
        {
            LOG_ERR( "Failed to convert asset %s %s", g_assetNames[assetType], derivedCreateInfo->name.c_str() );
            return false;
        }

        if ( !AssetCache::CacheAsset( assetType, cacheName, &asset ) )
        {
            LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], derivedCreateInfo->name.c_str(), cacheName.c_str() );
            return false;
        }

        return true;
    }
};

} // namespace PG
