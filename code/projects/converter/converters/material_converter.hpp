#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/material.hpp"

namespace PG
{

class MaterialConverter : public BaseAssetConverterTemplate<Material, MaterialCreateInfo>
{
public:
    MaterialConverter() : BaseAssetConverterTemplate( "Material", ASSET_TYPE_MATERIAL ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    ConvertDate IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG