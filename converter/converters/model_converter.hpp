#pragma once

#include "base_asset_converter.hpp"

namespace PG
{

class ModelConverter : public BaseAssetConverter
{
public:
    ModelConverter() : BaseAssetConverter( "Model", ASSET_TYPE_MODEL ) {}
    void Parse( const rapidjson::Value& value ) override;

protected:
    std::string GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const override;
    bool IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo ) override;
    bool ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const override;
};

} // namespace PG