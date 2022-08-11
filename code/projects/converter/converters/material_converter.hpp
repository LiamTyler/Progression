#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/material.hpp"

namespace PG
{

class MaterialConverter : public BaseAssetConverterTemplate<Material, MaterialCreateInfo>
{
public:
    MaterialConverter() : BaseAssetConverterTemplate( ASSET_TYPE_MATERIAL ) {}

protected:
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
    bool Convert( const std::string& assetName ) override;
};

} // namespace PG