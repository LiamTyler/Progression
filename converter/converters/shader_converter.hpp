#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/shader.hpp"

namespace PG
{

class ShaderConverter : public BaseAssetConverterTemplate<Shader, ShaderCreateInfo>
{
public:
    ShaderConverter() : BaseAssetConverterTemplate( "Shader", ASSET_TYPE_SHADER ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    bool IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG