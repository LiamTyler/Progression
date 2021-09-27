#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/gfx_image.hpp"

namespace PG
{

class GfxImageConverter : public BaseAssetConverterTemplate<GfxImage, GfxImageCreateInfo>
{
public:
    GfxImageConverter() : BaseAssetConverterTemplate( "Image", ASSET_TYPE_GFX_IMAGE ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    ConvertDate IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG