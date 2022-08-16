#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/material.hpp"

namespace PG
{

class MaterialConverter : public BaseAssetConverterTemplate<Material, MaterialCreateInfo>
{
public:
    MaterialConverter() : BaseAssetConverterTemplate( ASSET_TYPE_MATERIAL ) {}

    virtual void AddReferencedAssetsInternal( ConstDerivedInfoPtr& baseInfo ) override;

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
    bool ConvertInternal( ConstDerivedInfoPtr& createInfo ) override;
};

} // namespace PG