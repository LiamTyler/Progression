#pragma once

#include "base_asset_converter.hpp"

namespace PG
{

class ShaderConverter : public BaseAssetConverter
{
public:
    ShaderConverter () : BaseAssetConverter( "Shader", ASSET_TYPE_SHADER ) {}
    std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* info ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* info ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* info ) const override;
};

} // namespace PG