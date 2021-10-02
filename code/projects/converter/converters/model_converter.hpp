#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/model.hpp"

namespace PG
{

class ModelConverter : public BaseAssetConverterTemplate<Model, ModelCreateInfo>
{
public:
    ModelConverter() : BaseAssetConverterTemplate( "Model", ASSET_TYPE_MODEL ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG