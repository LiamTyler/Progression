#pragma once

#include "asset/types/script.hpp"
#include "base_asset_converter.hpp"
#include "shared/json_parsing.hpp"

namespace PG
{

class ScriptConverter : public BaseAssetConverterTemplate<Script, ScriptCreateInfo>
{
public:
    ScriptConverter() : BaseAssetConverterTemplate( ASSET_TYPE_SCRIPT ) {}

protected:
    std::string GetCacheNameInternal( ConstDerivedInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG