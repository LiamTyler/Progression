#pragma once

#include "base_asset_converter.hpp"

namespace PG
{

class MaterialConverter : public BaseAssetConverter
{
public:
    MaterialConverter() : BaseAssetConverter( "Material", ASSET_TYPE_MATERIAL ) {}
    std::shared_ptr<BaseAssetCreateInfo> Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* info ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* info ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* info ) const override;
};

} // namespace PG