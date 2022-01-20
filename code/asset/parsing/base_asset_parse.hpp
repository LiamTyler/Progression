#pragma once

#include "asset/asset_versions.hpp"
#include "asset/types/base_asset.hpp"
#include "shared/json_parsing.hpp"
#include "shared/logger.hpp"

#define PARSE_ERROR( ... ) { LOG_ERR( __VA_ARGS__ ); return false; }

namespace PG
{

class BaseAssetParser
{
public:
    using BaseInfoPtr = std::shared_ptr<BaseAssetCreateInfo>;
    using ConstBaseInfoPtr = const std::shared_ptr<const BaseAssetCreateInfo>&;

    const AssetType assetType;

    BaseAssetParser( AssetType inAssetType ) : assetType( inAssetType ) {}
    virtual ~BaseAssetParser() = default;

    virtual std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value, ConstBaseInfoPtr parent ) = 0;
};


template<typename DerivedInfo>
class BaseAssetParserTemplate : public BaseAssetParser
{
public:
    using InfoPtr = std::shared_ptr<DerivedInfo>;

    BaseAssetParserTemplate( AssetType inAssetType ) : BaseAssetParser( inAssetType ) {}
    virtual ~BaseAssetParserTemplate() = default;

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

protected:
    virtual bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) = 0;
};


extern const std::shared_ptr<BaseAssetParser> g_assetParsers[NUM_ASSET_TYPES];

} // namespace PG