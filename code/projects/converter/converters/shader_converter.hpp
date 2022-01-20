#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/shader.hpp"

namespace PG
{

class ShaderConverter : public BaseAssetConverterTemplate<Shader, ShaderCreateInfo>
{
public:
    ShaderConverter() : BaseAssetConverterTemplate( ASSET_TYPE_SHADER ) {}

protected:
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG