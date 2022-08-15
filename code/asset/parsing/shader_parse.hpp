#pragma once

#include "base_asset_parse.hpp"
#include "asset/types/shader.hpp"

namespace PG
{

class ShaderParser : public BaseAssetParserTemplate<ShaderCreateInfo>
{
public:
    ShaderParser() : BaseAssetParserTemplate( ASSET_TYPE_SHADER ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) override;
};


} // namespace PG