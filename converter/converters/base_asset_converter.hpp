#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include "asset_file_database.hpp"
#include "asset_cache.hpp"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/serializer.hpp"

#define PARSE_ERROR( ... ) { LOG_ERR( __VA_ARGS__ ); return false; }

namespace PG
{

struct ConverterConfigOptions
{
    bool force = false;
};

struct ConverterStatus
{
    bool error = false;
};

extern ConverterConfigOptions g_converterConfigOptions;
extern ConverterStatus g_converterStatus;

// IsAssetOutOfDate will take care of all the individual input files for each asset, but
// AddFastfileDependency needs to be called on each converted .ffi file
void AddFastfileDependency( const std::string& file );
void ClearAllFastfileDependencies();
time_t GetLatestFastfileDependency();
void FilenameSlashesToUnderscores( std::string& str );


class BaseAssetConverter
{
public:
    using BaseInfoPtr = std::shared_ptr<BaseAssetCreateInfo>;
    using ConstBaseInfoPtr = const std::shared_ptr<const BaseAssetCreateInfo>&;

    const std::string assetNameInJsonFile;
    const AssetType assetType;

    BaseAssetConverter( const std::string& inAssetName, AssetType inAssetType ) : assetNameInJsonFile( inAssetName ), assetType( inAssetType ) {}

    virtual std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value, ConstBaseInfoPtr parent ) = 0;
    virtual std::string GetCacheName( ConstBaseInfoPtr baseInfo ) = 0;
    virtual bool IsAssetOutOfDate( const std::string& assetName ) = 0;
    virtual bool Convert( const std::string& assetName ) = 0;
};


template< typename DerivedAsset, typename DerivedInfo>
class BaseAssetConverterTemplate : public BaseAssetConverter
{
public:
    BaseAssetConverterTemplate() = default;
    using InfoPtr = std::shared_ptr<DerivedInfo>;
    using ConstInfoPtr = const std::shared_ptr<const DerivedInfo>&;

    BaseAssetConverterTemplate( const std::string& inAssetName, AssetType inAssetType ) : BaseAssetConverter( inAssetName, assetType ) {}

    virtual std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value, ConstBaseInfoPtr parent ) override
    {
        auto info = std::make_shared<DerivedInfo>();
        if ( parent )
        {
            *info = *std::static_pointer_cast<const DerivedInfo>( parent );
        }
        const std::string assetName = value["name"].GetString();
        info->name = assetName;

        if ( !ParseInternal( value, info ) )
        {
            return nullptr;
        }
        return info;
    }

    virtual std::string GetCacheName( ConstBaseInfoPtr baseInfo )
    {
        return GetCacheNameInternal( std::static_pointer_cast<const DerivedInfo>( baseInfo ) );
    }

    virtual bool IsAssetOutOfDate( const std::string& assetName ) override
    {
        auto info = AssetDatabase::FindAssetInfo<DerivedInfo>( assetType, assetName );
        if ( !info )
        {
            LOG_ERR( "Scene requires asset %s of type %s, but no valid entry found in the database.", assetName.c_str(), assetNameInJsonFile.c_str() );
            g_converterStatus.error = true;
            return false;
        }

        if ( g_converterConfigOptions.force ) return true;

        time_t cachedTimestamp = AssetCache::GetAssetTimestamp( assetType, GetCacheName( info ) );
        if ( cachedTimestamp == NO_TIMESTAMP ) return true;

        return IsAssetOutOfDateInternal( info, cachedTimestamp );
    }

    virtual bool Convert( const std::string& assetName ) override
    {
        if ( !IsAssetOutOfDate( assetName ) )
        {
            return true;
        }

        auto info = AssetDatabase::FindAssetInfo<DerivedInfo>( assetType, assetName );
        LOG( "Converting %s '%s'...", assetNameInJsonFile.c_str(), info->name.c_str() );
        DerivedAsset asset;
        if ( !asset.Load( info.get() ) )
        {
            return false;
        }

        std::string cacheName = GetCacheName( info );
        if ( !AssetCache::CacheAsset( assetType, cacheName, &asset ) )
        {
            LOG_ERR( "Failed to cache asset %s %s (%s)", assetNameInJsonFile.c_str(), assetName.c_str(), cacheName.c_str() );
            return false;
        }

        return true;
    }

protected:
    virtual bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) = 0;
    virtual std::string GetCacheNameInternal( ConstInfoPtr info ) = 0;
    virtual bool IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) = 0;
};

} // namespace PG