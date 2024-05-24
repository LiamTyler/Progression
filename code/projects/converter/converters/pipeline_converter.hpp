#pragma once

#include "asset/types/pipeline.hpp"
#include "base_asset_converter.hpp"

namespace PG
{

class PipelineConverter : public BaseAssetConverterTemplate<Pipeline, PipelineCreateInfo>
{
public:
    PipelineConverter() : BaseAssetConverterTemplate( ASSET_TYPE_PIPELINE ) {}

    virtual void AddReferencedAssetsInternal( ConstDerivedInfoPtr& baseInfo ) override;

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG
