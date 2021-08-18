#pragma once

#include "base_asset_converter.hpp"

namespace PG
{

class MaterialFileConverter : public BaseAssetConverter
{
public:
    MaterialFileConverter() : BaseAssetConverter( "MatFile", ASSET_TYPE_MATERIAL ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* info ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* info ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* info ) const override;
};

} // namespace PG