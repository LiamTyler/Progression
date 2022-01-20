#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/gfx_image.hpp"

namespace PG
{

class GfxImageConverter : public BaseAssetConverterTemplate<GfxImage, GfxImageCreateInfo>
{
public:
    GfxImageConverter() : BaseAssetConverterTemplate( ASSET_TYPE_GFX_IMAGE ) {}

protected:
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG