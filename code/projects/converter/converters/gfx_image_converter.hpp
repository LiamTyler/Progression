#pragma once

#include "asset/types/gfx_image.hpp"
#include "base_asset_converter.hpp"

namespace PG
{

class GfxImageConverter : public BaseAssetConverterTemplate<GfxImage, GfxImageCreateInfo>
{
public:
    GfxImageConverter() : BaseAssetConverterTemplate( ASSET_TYPE_GFX_IMAGE ) {}

    virtual void AddReferencedAssetsInternal( ConstDerivedInfoPtr& baseInfo ) override;

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG
