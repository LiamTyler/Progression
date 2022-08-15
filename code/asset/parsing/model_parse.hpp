#pragma once

#include "base_asset_parse.hpp"
#include "asset/types/model.hpp"

namespace PG
{

class ModelParser : public BaseAssetParserTemplate<ModelCreateInfo>
{
public:
    ModelParser() : BaseAssetParserTemplate( ASSET_TYPE_MODEL ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info ) override;
};

} // namespace PG