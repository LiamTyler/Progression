#pragma once

#include "asset/types/textureset.hpp"
#include "base_asset_converter.hpp"
#include "shared/json_parsing.hpp"

namespace PG
{

class TexturesetConverter : public BaseAssetConverterTemplate<Textureset, TexturesetCreateInfo>
{
public:
    TexturesetConverter() : BaseAssetConverterTemplate( ASSET_TYPE_TEXTURESET ) {}

protected:
    virtual bool Convert( const std::string& assetName ) override;
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG