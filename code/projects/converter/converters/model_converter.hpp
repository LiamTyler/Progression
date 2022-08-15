#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/model.hpp"

namespace PG
{

class ModelConverter : public BaseAssetConverterTemplate<Model, ModelCreateInfo>
{
public:
    ModelConverter() : BaseAssetConverterTemplate( ASSET_TYPE_MODEL ) {}

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG