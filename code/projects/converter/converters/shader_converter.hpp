#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/shader.hpp"
#include "asset/shader_preprocessor.hpp"

namespace PG
{

void InitShaderIncludeCache();
void CloseShaderIncludeCache();

void AddIncludeCacheEntry( const std::string& cacheName, const ShaderCreateInfo* createInfo, const ShaderPreprocessOutput& preprocOutput );

class ShaderConverter : public BaseAssetConverterTemplate<Shader, ShaderCreateInfo>
{
public:
    ShaderConverter() : BaseAssetConverterTemplate( ASSET_TYPE_SHADER ) {}

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG