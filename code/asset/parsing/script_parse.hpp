#pragma once

#include "asset/types/script.hpp"
#include "base_asset_parse.hpp"

namespace PG
{

class ScriptParser : public BaseAssetParserTemplate<ScriptCreateInfo>
{
public:
    ScriptParser() : BaseAssetParserTemplate( ASSET_TYPE_SCRIPT ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
};

} // namespace PG