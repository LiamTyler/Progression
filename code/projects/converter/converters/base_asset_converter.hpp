#pragma once

#include "asset/asset_cache.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/file_dependency.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"

namespace PG
{

struct ConverterConfigOptions
{
    bool force = false;
};

enum class AssetStatus : uint8_t
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


class BaseAssetConverter
{
public:
    using BaseInfoPtr = std::shared_ptr<BaseAssetCreateInfo>;
    using ConstBaseInfoPtr = const std::shared_ptr<const BaseAssetCreateInfo>&;

    const AssetType assetType;

    BaseAssetConverter( AssetType inAssetType ) : assetType( inAssetType ) {}
    virtual ~BaseAssetConverter() = default;

    virtual std::string GetCacheName( ConstBaseInfoPtr baseInfo ) = 0;
    virtual AssetStatus IsAssetOutOfDate( const std::string& assetName ) = 0;
    virtual bool Convert( const std::string& assetName ) = 0;
};


template< typename DerivedAsset, typename DerivedInfo>
class BaseAssetConverterTemplate : public BaseAssetConverter
{
public:
    using InfoPtr = std::shared_ptr<DerivedInfo>;
    using ConstInfoPtr = const std::shared_ptr<const DerivedInfo>&;

    BaseAssetConverterTemplate( AssetType inAssetType ) : BaseAssetConverter( inAssetType ) {}
    virtual ~BaseAssetConverterTemplate() = default;

    virtual std::string GetCacheName( ConstBaseInfoPtr baseInfo )
    {
        return GetCacheNameInternal( std::static_pointer_cast<const DerivedInfo>( baseInfo ) );
    }

    virtual AssetStatus IsAssetOutOfDate( const std::string& assetName ) override
    {
        auto info = AssetDatabase::FindAssetInfo<DerivedInfo>( assetType, assetName );
        if ( !info )
        {
            LOG_ERR( "Scene requires asset %s of type %s, but no valid entry found in the database.", assetName.c_str(), g_assetNames[assetType] );
            return AssetStatus::ERROR;
        }

        if ( g_converterConfigOptions.force )
        {
            AddFastfileDependency( LATEST_TIMESTAMP );
            return AssetStatus::OUT_OF_DATE;
        }

        time_t cachedTimestamp = AssetCache::GetAssetTimestamp( assetType, GetCacheName( info ) );
        if ( cachedTimestamp == NO_TIMESTAMP )
        {
            AddFastfileDependency( LATEST_TIMESTAMP );
            return AssetStatus::OUT_OF_DATE;
        }
        AddFastfileDependency( cachedTimestamp );

        return IsAssetOutOfDateInternal( info, cachedTimestamp );
    }

    virtual bool Convert( const std::string& assetName ) override
    {
        auto info = AssetDatabase::FindAssetInfo<DerivedInfo>( assetType, assetName );
        LOG( "Converting out of date asset %s %s...", g_assetNames[assetType], info->name.c_str() );
        DerivedAsset asset;
        if ( !asset.Load( info.get() ) )
        {
            return false;
        }

        std::string cacheName = GetCacheName( info );
        if ( !AssetCache::CacheAsset( assetType, cacheName, &asset ) )
        {
            LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], assetName.c_str(), cacheName.c_str() );
            return false;
        }

        return true;
    }

protected:
    virtual std::string GetCacheNameInternal( ConstInfoPtr info ) = 0;
    virtual AssetStatus IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) = 0;
};

} // namespace PG