#pragma once

#include "asset/types/base_asset.hpp"
#include "asset/asset_versions.hpp"
#include "asset_cache.hpp"
#include "asset_file_database.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/file_dependency.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"

#define PARSE_ERROR( ... ) { LOG_ERR( __VA_ARGS__ ); return false; }

namespace PG
{

struct ConverterConfigOptions
{
    bool force = false;
};

enum class ConvertDate : uint8_t
{
    OUT_OF_DATE,
    UP_TO_DATE,
    ERROR
};

extern ConverterConfigOptions g_converterConfigOptions;

// IsAssetOutOfDate will take care of all the individual input files for each asset, but
// AddFastfileDependency needs to be called on each converted .ffi file
void AddFastfileDependency( const std::string& file );
void AddFastfileDependency( time_t timestamp );
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
    virtual ~BaseAssetConverter() = default;

    virtual std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value, ConstBaseInfoPtr parent ) = 0;
    virtual std::string GetCacheName( ConstBaseInfoPtr baseInfo ) = 0;
    virtual ConvertDate IsAssetOutOfDate( const std::string& assetName ) = 0;
    virtual bool Convert( const std::string& assetName ) = 0;
};


template< typename DerivedAsset, typename DerivedInfo>
class BaseAssetConverterTemplate : public BaseAssetConverter
{
public:
    using InfoPtr = std::shared_ptr<DerivedInfo>;
    using ConstInfoPtr = const std::shared_ptr<const DerivedInfo>&;

    BaseAssetConverterTemplate( const std::string& inAssetName, AssetType inAssetType ) : BaseAssetConverter( inAssetName, inAssetType ) {}
    virtual ~BaseAssetConverterTemplate() = default;

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

    virtual ConvertDate IsAssetOutOfDate( const std::string& assetName ) override
    {
        auto info = AssetDatabase::FindAssetInfo<DerivedInfo>( assetType, assetName );
        if ( !info )
        {
            LOG_ERR( "Scene requires asset %s of type %s, but no valid entry found in the database.", assetName.c_str(), assetNameInJsonFile.c_str() );
            return ConvertDate::ERROR;
        }

        if ( g_converterConfigOptions.force )
        {
            AddFastfileDependency( LATEST_TIMESTAMP );
            return ConvertDate::OUT_OF_DATE;
        }

        time_t cachedTimestamp = AssetCache::GetAssetTimestamp( assetType, GetCacheName( info ) );
        if ( cachedTimestamp == NO_TIMESTAMP )
        {
            AddFastfileDependency( LATEST_TIMESTAMP );
            return ConvertDate::OUT_OF_DATE;
        }
        AddFastfileDependency( cachedTimestamp );

        return IsAssetOutOfDateInternal( info, cachedTimestamp );
    }

    virtual bool Convert( const std::string& assetName ) override
    {
        ConvertDate status = IsAssetOutOfDate( assetName );
        if ( status == ConvertDate::UP_TO_DATE )
        {
            //LOG( "Asset %s is up to date", assetName.c_str() );
            return true;
        }
        else if ( status == ConvertDate::ERROR ) return false;

        auto info = AssetDatabase::FindAssetInfo<DerivedInfo>( assetType, assetName );
        LOG( "Converting out of date asset %s %s...", assetNameInJsonFile.c_str(), info->name.c_str() );
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
    virtual ConvertDate IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) = 0;
};

} // namespace PG