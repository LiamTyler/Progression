#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/gfx_image.hpp"
#include "shared/json_parsing.hpp"
#include "asset/types/script.hpp"

namespace PG
{

class ScriptConverter : public BaseAssetConverterTemplate<Script, ScriptCreateInfo>
{
public:
    ScriptConverter() : BaseAssetConverterTemplate( "Script", ASSET_TYPE_SCRIPT ) {}

protected:
    bool ParseInternal( const rapidjson::Value& value, InfoPtr info ) override;
    std::string GetCacheNameInternal( ConstInfoPtr info ) override;
    ConvertDate IsAssetOutOfDateInternal( ConstInfoPtr info, time_t cacheTimestamp ) override;
};

} // namespace PG