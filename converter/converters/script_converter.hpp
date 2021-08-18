#pragma once

#include "base_asset_converter.hpp"
#include "asset/types/gfx_image.hpp"
#include "utils/json_parsing.hpp"

namespace PG
{

class ScriptConverter : public BaseAssetConverter
{
public:
    ScriptConverter() : BaseAssetConverter( "Script", ASSET_TYPE_SCRIPT ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* info ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* info ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* info ) const override;
};

} // namespace PG