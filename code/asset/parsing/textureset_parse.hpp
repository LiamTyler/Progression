#pragma once

#include "base_asset_parse.hpp"
#include "asset/types/textureset.hpp"

namespace PG
{

class TexturesetParser : public BaseAssetParserTemplate<TexturesetCreateInfo>
{
public:
    TexturesetParser() : BaseAssetParserTemplate( ASSET_TYPE_TEXTURESET ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
};


} // namespace PG