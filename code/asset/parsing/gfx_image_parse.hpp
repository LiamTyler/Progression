#pragma once

#include "asset/parsing/base_asset_parse.hpp"
#include "asset/types/gfx_image.hpp"

namespace PG
{

class GfxImageParser : public BaseAssetParserTemplate<GfxImageCreateInfo>
{
public:
    GfxImageParser() : BaseAssetParserTemplate( ASSET_TYPE_GFX_IMAGE ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
};

} // namespace PG