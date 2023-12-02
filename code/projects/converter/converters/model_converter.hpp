#pragma once

#include "asset/types/model.hpp"
#include "base_asset_converter.hpp"

namespace PG
{

class ModelConverter : public BaseAssetConverterTemplate<Model, ModelCreateInfo>
{
public:
    ModelConverter() : BaseAssetConverterTemplate( ASSET_TYPE_MODEL ) {}

    virtual void AddReferencedAssetsInternal( ConstDerivedInfoPtr& baseInfo ) override;

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG
