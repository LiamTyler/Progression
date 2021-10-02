#pragma once

#include "asset/types/script.hpp"
#include "base_asset_converter.hpp"
#include "shared/json_parsing.hpp"

namespace PG
{

class ScriptConverter : public BaseAssetConverterTemplate<Script, ScriptCreateInfo>
{
public:
    ScriptConverter() : BaseAssetConverterTemplate( "Script", ASSET_TYPE_SCRIPT ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    AssetStatus IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG