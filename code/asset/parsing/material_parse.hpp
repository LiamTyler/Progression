#pragma once

#include "base_asset_parse.hpp"
#include "asset/types/material.hpp"

namespace PG
{

class MaterialParser : public BaseAssetParserTemplate<MaterialCreateInfo>
{
public:
    MaterialParser() : BaseAssetParserTemplate( ASSET_TYPE_MATERIAL ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) override;
};

} // namespace PG