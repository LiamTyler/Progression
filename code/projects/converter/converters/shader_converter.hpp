#pragma once

#include "asset/shader_preprocessor.hpp"
#include "asset/types/shader.hpp"
#include "base_asset_converter.hpp"

namespace PG
{

void InitShaderIncludeCache();
void CloseShaderIncludeCache();

void AddIncludeCacheEntry( const ShaderCreateInfo* createInfo, const ShaderPreprocessOutput& preprocOutput );

class ShaderConverter : public BaseAssetConverterTemplate<Shader, ShaderCreateInfo>
{
public:
    ShaderConverter() : BaseAssetConverterTemplate( ASSET_TYPE_SHADER ) {}

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG
